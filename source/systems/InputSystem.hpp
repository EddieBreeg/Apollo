#pragma once

#include <PCH.hpp>
#include <entt/entity/fwd.hpp>

namespace brk{
	class GameTime;
}

namespace brk::inputs {
	class BRK_API System
	{
	public:
		System() = default;
		~System() = default;

		void Update(entt::registry&, const GameTime&);
	
	private:
	};
}