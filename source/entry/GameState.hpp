#pragma once

#include <PCH.hpp>

/** \file GameState.hpp
 * \class IGameState
 */

namespace apollo {
	enum class EAppResult : int8;

	class App;
	struct EntryPoint;

	/**
	 * \brief The state of our game, both initialization code and any data we may require
	 * \sa \ref entry
	 * \sa apollo::EntryPoint
	 */
	struct IGameState
	{
		virtual ~IGameState() = default;

		/**
		 * \brief Called at the end of the initialization phase, \b after all the code managers and
		 * systems have been initialized.
		 * \details This function should perform all specific initialization operations your game
		 * requires, like registering components and systems, loading custom files like user
		 * preferences etc
		 */
		virtual EAppResult OnInit(const EntryPoint&, App&) { return static_cast<EAppResult>(0); }
		/**
		 * \brief Called when the application is about to terminate, \b before all the core managers
		 * and ECS systems get destroyed
		 */
		virtual void OnQuit(App&) {}
	};
} // namespace apollo