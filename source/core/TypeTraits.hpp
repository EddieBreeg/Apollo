#pragma once

#include <PCH.hpp>

namespace brk::meta {
	template <class T>
	concept NoThrowMovable = std::is_nothrow_move_constructible_v<T> &&
							 std::is_nothrow_move_assignable_v<T>;

	template <class T>
	struct MemberObjectTraits;

	template <class T, class M>
	struct MemberObjectTraits<M T::*>
	{
		using ClassType = T;
		using MemberType = M;
	};
} // namespace brk::meta