#pragma once

/** \file Entry.hpp */

/**
 * \page entry Entry point

 * In Apollo, the entry point is a little special compare to what you might be used to in a normal
 * C++ application. Instead of using the main function, the application entry point is defined using
 * the \ref apollo::GetEntryPoint "GetEntryPoint" function. This function returns an \ref
 * apollo::EntryPoint "EntryPoint" object used to provide details about how the application should
 * be initialized. The reasonning behind this design choice is that the engine initialization is
 * sensitive and should be controlled by the engine itself, while still leaving room for
 * customization on the user's part.
 *
 *
 * Here is an example of how you could declare an entry point for your game:
 * \include{strip} example/main.cpp

 * \sa apollo::GetEntryPoint
 * \sa apollo::EntryPoint
 */

#include <PCH.hpp>
#include <filesystem>
#include <span>

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
		/**
		 * The application name. This will be used as the window title.
		 */
		const char* m_AppName = nullptr;
		uint32 m_WindowWidth = 1280;
		uint32 m_WindowHeight = 720;

		/**
		 * \brief Initialization callback.
		 * This function should perform all specific initialization operations your game requires,
		 * like registering components and systems.
		 * \note This function only gets involved after all the core systems and managers have been
		 * initialized. This means you can rely on these core elements (e.g. the \ref
		 * apollo::rdr::Context "Rendering Context") being available if you need them.
		 */
		EAppResult (*m_OnInit)(const EntryPoint&, App&) = nullptr;

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