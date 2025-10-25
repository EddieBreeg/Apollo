#pragma once

#include <PCH.hpp>
#include <entt/entity/fwd.hpp>

namespace apollo {
	class GameTime;
	class App;
} // namespace apollo

namespace apollo::inputs {
	class APOLLO_API System
	{
	public:
		System(App& app);
		~System() = default;

		void Update(entt::registry&, const GameTime&);

	private:
		App& m_App;
	};

	APOLLO_API const bool *GetKeyStates() noexcept;
} // namespace apollo::inputs