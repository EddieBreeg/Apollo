#pragma once

/** \file Entry.hpp */
/**
 * \dir source/entry
 * \brief Entry point of the engine
 * \sa \ref entry
 */

#include <PCH.hpp>
#include <filesystem>
#include <memory>
#include <span>

#include "GameState.hpp"

namespace apollo::mt {
	class ThreadPool;
}

namespace apollo::rdr {
	class GPUDevice;
}

namespace apollo {
	class App;
	class IAssetManager;

	/**
	* \addtogroup global-structure
	@{
	 */

	/// Application result code
	enum class EAppResult : int8
	{
		Continue = 0, //!< Tells the application to continue execution
		Success,	  //!< Execution ended successfully
		Failure,	  //!< Execution ended with an error
	};

	/**
	 * \brief Entry point definition object.
	 * The gets passed to the application constructor during the initialization phase.
	 * \sa apollo::App
	 */
	struct EntryPoint
	{
		const char* m_AppName = nullptr; /*!< The application name. This will be used as the window
											title. */
		uint32 m_WindowWidth = 1280;
		uint32 m_WindowHeight = 720;

		/** \brief The game's state.
		 * \important Must always be initialized
		 */
		std::unique_ptr<IGameState> m_GameState;

		/**
		 * If non-empty, this path should point to the root folder containing the asset metadata
		 * file for the project. Ignored if empty.
		 */
		std::filesystem::path m_AssetRoot;
	};

	/// %Asset manager initialization function, used internally by the app during initialization.
	using AssetManagerInitFunc = IAssetManager&(
		const std::filesystem::path& settings,
		rdr::GPUDevice& gpuDevice,
		mt::ThreadPool& threadPool);

	/**
	 * \brief User-provided entry point definition
	 * \param args: The arguments passed to the application, including the executable path
	 * \returns The entry point definition. See \ref apollo::EntryPoint "EntryPoint" for
	 * details.
	 */
	extern EntryPoint GetEntryPoint(std::span<const char*> args);

	/** @} */
} // namespace apollo