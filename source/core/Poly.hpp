#pragma once

#include <PCH.hpp>

/** \file Poly.hpp */

/**
 * \namespace apollo::poly
 * \brief Type-erasure support
 */
namespace apollo::poly {
	/**
	 * \brief Type-erased destructor
	 */
	template <class T>
	void Destroy(void* ptr)
	{
		if constexpr (!std::is_trivially_destructible_v<T>)
		{
			if (ptr)
				static_cast<T*>(ptr)->~T();
		}
	}

	/**
	 * \brief Type-erased delete function
	 */

	template <class T>
	void Delete(void* ptr)
	{
		if (ptr)
			delete static_cast<T*>(ptr);
	}

	/**
	 * \brief Type-erased functor invocation function
	 */
	template <class F, class... Args>
	decltype(auto) Invoke(void* ptr, Args&&... args) requires(std::is_invocable_v<F, Args...>)
	{
		return (*static_cast<F*>(ptr))(std::forward<Args>(args)...);
	}
} // namespace apollo::poly