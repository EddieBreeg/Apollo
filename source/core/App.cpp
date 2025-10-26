#include "App.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <asset/AssetManager.hpp>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <core/Log.hpp>
#include <ecs/Manager.hpp>
#include <entry/Entry.hpp>
#include <imgui.h>
#include <rendering/Context.hpp>

namespace {
	ImGuiContext* InitImGui(SDL_Window* window, SDL_GPUDevice* device)
	{
		auto* const ctx = ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;	  // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;	  // Enable Multi-Viewport / Platform
															  // Windows

		ImGui_ImplSDL3_InitForSDLGPU(window);
		ImGui_ImplSDLGPU3_InitInfo init_info = {};
		init_info.Device = device;
		init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
		init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
		init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
		init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
		ImGui_ImplSDLGPU3_Init(&init_info);
		return ctx;
	}

	void ShutdownImGui()
	{
		ImGui_ImplSDL3_Shutdown();
		ImGui_ImplSDLGPU3_Shutdown();
		ImGui::DestroyContext();
	}
} // namespace

namespace apollo {
	std::unique_ptr<App> App::s_Instance;

	void RegisterCoreSystems(App& app, ecs::Manager& manager);

	App::App(const EntryPoint& entry)
		: m_EntryPoint(entry)
		, m_Result(EAppResult::Continue)
	{
		DEBUG_CHECK(SDL_Init(SDL_INIT_VIDEO))
		{
			APOLLO_LOG_CRITICAL("Failed to init SDL: {}", SDL_GetError());
			return;
		}

		spdlog::set_level(spdlog::level::trace);
		APOLLO_LOG_INFO(
			"Starting Apollo version {}.{}.{}",
			APOLLO_VERSION_MAJOR,
			APOLLO_VERSION_MINOR,
			APOLLO_VERSION_PATCH);

		WindowSettings winSettings{
			.m_Title = entry.m_AppName,
			.m_Width = entry.m_WindowWidth,
			.m_Height = entry.m_WindowHeight,
		};
#ifdef APOLLO_DEV
		winSettings.m_Resizeable = true;
#endif

		DEBUG_CHECK(m_Window = Window(winSettings))
		{
			m_Result = EAppResult::Failure;
			return;
		}

#ifdef APOLLO_DEV
		m_RenderContext = &rdr::Context::Init(rdr::EBackend::Default, m_Window, true);
#else
		m_RenderContext = &rdr::Context::Init(rdr::EBackend::Default, m_Window, false);
#endif
		m_AssetManager = &AssetManager::Init(entry.m_AssetManagerSettings, m_RenderContext->GetDevice());

		m_ImGuiContext = InitImGui(m_Window.GetHandle(), m_RenderContext->GetDevice().GetHandle());

		m_ECSManager = &ecs::Manager::Init();
		RegisterCoreSystems(*this, *m_ECSManager);

		if (m_EntryPoint.m_OnInit)
			m_Result = m_EntryPoint.m_OnInit(m_EntryPoint, *this);

		if (m_Result != EAppResult::Continue)
			return;
	}

	EAppResult App::Run()
	{
		m_ECSManager->PostInit();
		m_GameTime.Reset();

		for (;;)
		{
			m_Result = Update();
			if (m_Result != EAppResult::Continue)
				break;
		}
		return m_Result;
	}

	void App::RequestAppQuit(bool success) noexcept
	{
		m_Result = success ? EAppResult::Success : EAppResult::Failure;
	}

	EAppResult App::Update()
	{
		ImGui_ImplSDLGPU3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		m_RenderContext->BeginFrame();
		m_AssetManager->Update();

		m_ECSManager->Update(m_GameTime);
		m_RenderContext->ImGuiRenderPass();

		ImGui::EndFrame();

		m_RenderContext->EndFrame();

		// Update and Render additional Platform Windows
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
		m_GameTime.Update();

		return m_Result;
	}

	App::~App()
	{
		APOLLO_LOG_INFO("Shutting down...");
		ShutdownImGui();
		ecs::Manager::Shutdown();
		AssetManager::Shutdown();
		rdr::Context::Shutdown();
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}
} // namespace apollo