#pragma once

#include <PCH.hpp>
#include <memory>

/** \file Singleton.hpp */

namespace apollo {
	/**
	 * \brief Singleton class helper.
	 * This helper is used as a base for most singletons in the engine.
	 * \tparam T: The derived singleton type. \b T is expected to declare a static instance of type
	 * std::unique_ptr<T> called s_Instance.
	 * See the App class for an example of how to use this class.
	 */
	template <class T>
	class Singleton
	{
	public:
		/**
		 * \brief Creates the instance, or returns the pre-existing one.
		 * \param args: The arguments to forward to the constructor.
		 */
		template <class... Args>
		static T& Init(Args&&... args)
		{
			if (T::s_Instance)
				return *T::s_Instance;

			T::s_Instance.reset(new T{ std::forward<Args>(args)... });
			return *T::s_Instance;
		}

		[[nodiscard]] static T* GetInstance() { return T::s_Instance.get(); }
		/// Destroys the instance
		static void Shutdown() { T::s_Instance.reset(); }

	protected:
		Singleton() = default;
		Singleton(const Singleton&) = delete;
		Singleton(Singleton&&) = delete;
		~Singleton() = default;
	};
} // namespace apollo