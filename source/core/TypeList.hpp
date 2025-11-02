#pragma once

#include <PCH.hpp>

namespace apollo {
	template<class... T>
	struct TypeList
	{
		static constexpr uint32 Size = sizeof...(T);
	};
}