#pragma once

#include <PCH.hpp>
#include <memory>

namespace brk {
	/**
	* Singleton class helper. T must declare a static s_Instance of type std::unique_ptr<T>
	 */
	template <class T>
	class Singleton
	{
	public:
		template <class... Args>
		static T& Init(Args&&... args)
		{
			if (T::s_Instance)
				return *T::s_Instance;

			T::s_Instance.reset(new T{ std::forward<Args>(args)... });
			return *T::s_Instance;
		}

		static T* GetInstance() { return T::s_Instance.get(); }
		static void Shutdown() { T::s_Instance.reset(); }

	protected:
		Singleton() = default;
		Singleton(const Singleton&) = delete;
		Singleton(Singleton&&) = delete;
		~Singleton() = default;
	};
} // namespace brk