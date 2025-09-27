#pragma once

#include <PCH.hpp>
#include <entt/entity/fwd.hpp>

namespace brk {
	class GameTime;
	class App;
} // namespace brk

namespace brk::inputs {
	class BRK_API System
	{
	public:
		System(App& app);
		~System() = default;

		void Update(entt::registry&, const GameTime&);

	private:
		App& m_App;
	};
} // namespace brk::inputs