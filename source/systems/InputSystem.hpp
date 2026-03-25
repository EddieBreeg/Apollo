#pragma once

#include <PCH.hpp>
#include <entt/entity/fwd.hpp>

/** \file InputSystem.hpp */

namespace apollo {
	class GameTime;
	class App;
} // namespace apollo

/**
 * \namespace apollo::inputs
 * \brief Inputs management (keyboard/mouse/window events etc)
 */
namespace apollo::inputs {
	/**
	 * \brief System in charge of emitting input events
	 * \sa The components defined in InputEventComponents.hpp
	 */
	class APOLLO_API System
	{
	public:
		System(App& app);
		~System() = default;

		void Update(entt::registry&, const GameTime&);

	private:
		App& m_App;
	};

	APOLLO_API const bool* GetKeyStates() noexcept;
} // namespace apollo::inputs