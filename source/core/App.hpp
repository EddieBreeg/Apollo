#pragma once

#include <PCH.hpp>

#include "GameTime.hpp"
#include "Singleton.hpp"
#include "ThreadPool.hpp"
#include "Window.hpp"
#include <entry/Entry.hpp>

/** \file App.hpp */

struct ImGuiContext;

/**
 * \namespace apollo
 * \brief The main namespace. Everything that belongs to the engine lives here
 */
namespace apollo {
	namespace ecs {
		class Manager;
	}

	namespace rdr {
		class Context;
		class ShaderCompiler;
	} // namespace rdr

	enum class EAppResult : int8;

	struct EntryPoint;

	/**
	 * \defgroup global-structure General application structure
	 * @{
	 */

	/**
	 * \brief Main App class.
	 * The App class handles the main program loop, as well as initialization/clean-up code. It gets
	 * initialized in the `main` function and is destroyed at the very end of the program.
	 */
	class App : public Singleton<App>
	{
	public:
		/**
		 * \brief Shuts down the core managers before exiting
		 */
		APOLLO_API ~App();
		/**
		 * \returns The current application result code. This will always be EAppResult::Continue
		 * until either a termination even was received, or a critical error has occurred.
		 */
		[[nodiscard]] EAppResult GetResultCode() const noexcept { return m_Result; }
		[[nodiscard]] Window& GetMainWindow() noexcept { return m_Window; }
		[[nodiscard]] ImGuiContext* GetImGuiContext() noexcept { return m_ImGuiContext; }
		[[nodiscard]] rdr::Context* GetRenderContext() noexcept { return m_RenderContext; }
		[[nodiscard]] mt::ThreadPool& GetThreadPool() noexcept { return m_MainThreadPool; }
		[[nodiscard]] IGameState& GetGameState() const noexcept
		{
			return *m_EntryPoint.m_GameState;
		}
		[[nodiscard]] static APOLLO_API rdr::ShaderCompiler& GetShaderCompiler() noexcept;

		/**
		 * \brief Main loop.
		 * Blocks until the program comes to an end, e.g. after calling RequestAppQuit()
		 */
		APOLLO_API EAppResult Run();

		/**
		 * \brief Internally requests program termination.
		 * \param success: Should be `true` if the app terminated normally, `false` otherwise
		 * \details This tells the engine to end the main program loop. The current iteration of the
		 * loop will still complete.
		 */
		APOLLO_API void RequestAppQuit(bool success = true) noexcept;


	private:
		APOLLO_API EAppResult Update();

		APOLLO_API App(const EntryPoint& entry, AssetManagerInitFunc* initAssetManager);
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

	/** @} */
} // namespace apollo