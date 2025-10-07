#pragma once

#include <PCH.hpp>

#include "GameTime.hpp"
#include "Singleton.hpp"
#include "ThreadPool.hpp"
#include "Window.hpp"

struct ImGuiContext;

namespace brk {
	class AssetManager;

	namespace ecs {
		class Manager;
	}

	namespace rdr {
		class Renderer;
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
		BRK_API ~App();
		[[nodiscard]] EAppResult GetResultCode() const noexcept { return m_Result; }
		[[nodiscard]] Window& GetMainWindow() noexcept { return m_Window; }
		[[nodiscard]] ImGuiContext* GetImGuiContext() noexcept { return m_ImGuiContext; }

		// Main loop. Blocks until the program comes to an end, e.g. after calling RequestAppQuit()
		BRK_API EAppResult Run();

		// Internally requests program termination. The loop will stop once the current iteration is
		// done
		BRK_API void RequestAppQuit() noexcept;

		[[nodiscard]] mt::ThreadPool& GetThreadPool() noexcept { return m_MainThreadPool; }

	private:
		BRK_API EAppResult Update();

		BRK_API App(const EntryPoint& entry);
		friend class Singleton<App>;

		static BRK_API std::unique_ptr<App> s_Instance;

		const EntryPoint& m_EntryPoint;
		Window m_Window;
		rdr::Renderer* m_Renderer = nullptr;
		ecs::Manager* m_ECSManager = nullptr;
		AssetManager* m_AssetManager = nullptr;
		ImGuiContext* m_ImGuiContext = nullptr;

		EAppResult m_Result;
		GameTime m_GameTime;
		mt::ThreadPool m_MainThreadPool;
	};
} // namespace brk