#pragma once

#include <PCH.hpp>

namespace brk::meta {
	template <class T>
	concept NoThrowMovable = std::is_nothrow_move_constructible_v<T> &&
							 std::is_nothrow_move_assignable_v<T>;
}