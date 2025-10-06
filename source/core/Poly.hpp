#pragma once

#include <PCH.hpp>

namespace brk::poly {
	template <class T>
	void Destroy(void* ptr)
	{
		if constexpr (!std::is_trivially_destructible_v<T>)
		{
			if (ptr)
				static_cast<T*>(ptr)->~T();
		}
	}

	template <class T>
	void Delete(void* ptr)
	{
		if (ptr)
			delete static_cast<T*>(ptr);
	}

	template <class F, class... Args>
	decltype(auto) Invoke(void* ptr, Args&&... args) requires(std::is_invocable_v<F, Args...>)
	{
		return (*static_cast<F*>(ptr))(std::forward<Args>(args)...);
	}
} // namespace brk::poly