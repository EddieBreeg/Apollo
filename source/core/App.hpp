#pragma once

#include <PCH.hpp>

#include "GameTime.hpp"
#include "Singleton.hpp"
#include "ThreadPool.hpp"
#include "Window.hpp"

struct ImGuiContext;

namespace apollo {
	class IAssetManager;

	namespace ecs {
		class Manager;
	}

	namespace rdr {
		class Context;
	}

	enum class EAppResult : int8;

	struct EntryPoint;

	/**
	 * Main App class. This gets initialised in the entry point, and destroyed at the end of the
	 * program
	 */
	class App : public Singleton<App>
	{
	public:
		APOLLO_API ~App();
		[[nodiscard]] EAppResult GetResultCode() const noexcept { return m_Result; }
		[[nodiscard]] Window& GetMainWindow() noexcept { return m_Window; }
		[[nodiscard]] ImGuiContext* GetImGuiContext() noexcept { return m_ImGuiContext; }
		[[nodiscard]] rdr::Context* GetRenderContext() noexcept { return m_RenderContext; }

		// Main loop. Blocks until the program comes to an end, e.g. after calling RequestAppQuit()
		APOLLO_API EAppResult Run();

		// Internally requests program termination. The loop will stop once the current iteration is
		// done
		APOLLO_API void RequestAppQuit(bool success = true) noexcept;

		[[nodiscard]] mt::ThreadPool& GetThreadPool() noexcept { return m_MainThreadPool; }

	private:
		APOLLO_API EAppResult Update();

		APOLLO_API App(const EntryPoint& entry);
		friend class Singleton<App>;

		static APOLLO_API std::unique_ptr<App> s_Instance;

		const EntryPoint& m_EntryPoint;
		Window m_Window;
		rdr::Context* m_RenderContext = nullptr;
		ecs::Manager* m_ECSManager = nullptr;
		IAssetManager* m_AssetManager = nullptr;
		ImGuiContext* m_ImGuiContext = nullptr;

		EAppResult m_Result;
		GameTime m_GameTime;
		mt::ThreadPool m_MainThreadPool;
	};
} // namespace apollo