#include "InputSystem.hpp"
#include "InputEventComponents.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <backends/imgui_impl_sdl3.h>
#include <core/App.hpp>
#include <entry/Entry.hpp>
#include <entt/entity/registry.hpp>

namespace {
	apollo::EAppResult ProcessEvents(SDL_Window* mainWindow, entt::registry& entityWorld)
	{
		SDL_Event evt;
		const auto mainWindowId = SDL_GetWindowID(mainWindow);

		while (SDL_PollEvent(&evt))
		{
			ImGui_ImplSDL3_ProcessEvent(&evt);
			switch (evt.type)
			{
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				if (evt.window.windowID != mainWindowId)
					return apollo::EAppResult::Continue;
				[[fallthrough]];
			case SDL_EVENT_QUIT: return apollo::EAppResult::Success;

			case SDL_EVENT_WINDOW_RESIZED:
				if (evt.window.windowID == mainWindowId)
				{
					entityWorld.emplace<apollo::inputs::WindowResizeEventComponent>(
						entityWorld.create(),
						uint32(evt.window.data1),
						uint32(evt.window.data2));
				}
				break;
			default: continue;
			}
		}

		return apollo::EAppResult::Continue;
	}
} // namespace

namespace apollo::inputs {
	System::System(App& app)
		: m_App(app)
	{}

	void System::Update(entt::registry& world, const GameTime&)
	{
		for (entt::entity event : world.view<const WindowResizeEventComponent>())
			world.destroy(event);

		const EAppResult result = ProcessEvents(m_App.GetMainWindow().GetHandle(), world);
		if (result != EAppResult::Continue)
			m_App.RequestAppQuit();
	}
} // namespace apollo::inputs