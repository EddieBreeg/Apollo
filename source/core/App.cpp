#include "App.hpp"
#include "Log.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <ecs/Manager.hpp>
#include <entry/Entry.hpp>
#include <rendering/Renderer.hpp>

namespace {
	brk::EAppResult ProcessEvents(SDL_Window* mainWindow)
	{
		SDL_Event evt;
		while (SDL_PollEvent(&evt))
		{
			switch (evt.type)
			{
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				if (evt.window.windowID != SDL_GetWindowID(mainWindow))
					return brk::EAppResult::Continue;
				[[fallthrough]];
			case SDL_EVENT_QUIT: return brk::EAppResult::Success;
			default: continue; ;
			}
		}

		return brk::EAppResult::Continue;
	}
} // namespace

namespace brk {
	std::unique_ptr<App> App::s_Instance;

	void RegisterCoreSystems(ecs::Manager& manager);

	App::App(const EntryPoint& entry)
		: m_EntryPoint(entry)
		, m_Result(EAppResult::Continue)
	{
		DEBUG_CHECK(SDL_Init(SDL_INIT_VIDEO))
		{
			BRK_LOG_CRITICAL("Failed to init SDL: {}", SDL_GetError());
			return;
		}

		spdlog::set_level(spdlog::level::trace);

		WindowSettings winSettings{
			.m_Title = entry.m_AppName,
			.m_Width = entry.m_WindowWidth,
			.m_Height = entry.m_WindowHeight,
		};
#ifdef BRK_DEV
		winSettings.m_Resizeable = true;
#endif

		DEBUG_CHECK(m_Window = Window(winSettings))
		{
			m_Result = EAppResult::Failure;
			return;
		}

#ifdef BRK_DEV
		m_Renderer = &rdr::Renderer::Init(rdr::EBackend::Default, m_Window, true);
#else
		m_Renderer = &rdr::Renderer::Init(rdr::EBackend::Default, m_Window, false);
#endif
		m_ECSManager = &ecs::Manager::Init();
		RegisterCoreSystems(*m_ECSManager);

		BRK_LOG_INFO(
			"Starting Breakout version {}.{}.{}",
			BRK_VERSION_MAJOR,
			BRK_VERSION_MINOR,
			BRK_VERSION_PATCH);

		if (m_EntryPoint.m_OnInit)
			m_Result = m_EntryPoint.m_OnInit(m_EntryPoint, *this);

		if (m_Result != EAppResult::Continue)
			return;
	}

	EAppResult App::Run()
	{
		for (;;)
		{
			m_Result = ProcessEvents(m_Window.GetHandle());
			if (m_Result != EAppResult::Continue)
				return m_Result;

			m_ECSManager->Update();
		}
		return m_Result;
	}

	App::~App()
	{
		BRK_LOG_INFO("Shutting down...");
		ecs::Manager::Shutdown();
		rdr::Renderer::Shutdown();
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}
} // namespace brk