#pragma once

#include <PCH.hpp>
#include <coroutine>

#include "Assert.hpp"

/** \file Coroutine.hpp */

/**
 * \namespace apollo::coro
 * \brief Coroutine support
 * \sa The [standard's documentation](https://en.cppreference.com/w/cpp/language/coroutines.html)
 * for coroutines
 */
namespace apollo::coro {
	/**
	 * \brief Casts from one coroutine handle type to another, assuming the conversion is well
	 * defined
	 */
	template <class To, class From>
	[[nodiscard]] std::coroutine_handle<To> StaticHandleCast(std::coroutine_handle<From> h) noexcept
		requires(requires(From* x) { static_cast<To*>(x); })
	{
		return std::coroutine_handle<To>::from_address(h.address());
	}

	/**
	 * \brief Basic Coroutine class
	 * \tparam P: The coroutine's promise type.
	 */
	template <class P = void>
	class Coroutine
	{
	public:
		using promise_type = P;
		using HandleType = std::coroutine_handle<P>;

		constexpr Coroutine() noexcept = default;
		explicit Coroutine(HandleType handle) noexcept
			: m_Handle(handle)
		{}
		constexpr Coroutine(Coroutine&& other) noexcept
			: m_Handle(other.m_Handle)
		{
			other.m_Handle = nullptr;
		}
		constexpr Coroutine& operator=(Coroutine&& other) noexcept
		{
			Swap(other);
			return *this;
		}

		template <class O>
		constexpr Coroutine(Coroutine<O>&& other) noexcept
			requires(!std::is_same_v<O, P> && requires(O * x) { static_cast<P*>(x); })
			: m_Handle(StaticHandleCast<P>(other.m_Handle))
		{
			other.m_Handle = nullptr;
		}

		/** \name Member access operator
		 * \brief Allows easy access to the underlying promise
		 * \returns \code &m_Handle.promise() \endcode
		 * @{ */
		[[nodiscard]] P* operator->() noexcept { return &m_Handle.promise(); }
		[[nodiscard]] const P* operator->() const noexcept { return &m_Handle.promise(); }
		/** @} */

		/**
		 * \brief Invokes/resumes the coroutine
		 * \details If the internal handle points to a currently suspended coroutine, resumes it.
		 * Otherwise returns true immediately. Asserts if *this does not refer to a valid coroutine
		 * object.
		 * \returns true if the underlying coroutine is null or has reached its final suspension
		 * point, false otherwise.
		 */
		bool operator()()
		{
			APOLLO_ASSERT((bool)m_Handle, "Tried to resume a null coroutine");

			if (m_Handle.done())
				return true;

			m_Handle.resume();
			return m_Handle.done();
		}

		/**
		 * \brief Indicates whether the current object points to valid coroutine which has reached
		 * its final suspension point.
		 */
		[[nodiscard]] constexpr bool IsDone() const noexcept { return m_Handle && m_Handle.done(); }
		[[nodiscard]] constexpr operator bool() const noexcept { return (bool)m_Handle; }

		[[nodiscard]] constexpr bool operator==(HandleType handle) const noexcept
		{
			return m_Handle == handle;
		}
		[[nodiscard]] constexpr bool operator!=(HandleType handle) const noexcept
		{
			return m_Handle != handle;
		}

		constexpr void Reset(HandleType handle = nullptr) noexcept
		{
			if (m_Handle)
				m_Handle.destroy();
			m_Handle = handle;
		}

		/**
		 * \brief Replaces the internal handle and returns the old one.
		 */
		[[nodiscard("Discarding the returned handle would cause a leak")]] constexpr HandleType Release(
			HandleType other = nullptr) noexcept
		{
			auto temp = m_Handle;
			m_Handle = other;
			return temp;
		}

		constexpr void Swap(Coroutine& other) noexcept { std::swap(m_Handle, other.m_Handle); }

		constexpr ~Coroutine()
		{
			if (m_Handle)
				m_Handle.destroy();
		}

	protected:
		template <class O>
		friend class Coroutine;
		HandleType m_Handle;
	};

	/**
	 * \brief Utility promise type which does nothing.
	 * \tparam EagerStart: Whether the coroutine should be started immediately or suspended upon
	 * creation.
	 */
	template <bool EagerStart = false>
	struct NoopPromise
	{
		Coroutine<NoopPromise> get_return_object() noexcept
		{
			return Coroutine{ std::coroutine_handle<NoopPromise>::from_promise(*this) };
		}
		template <class To>
		Coroutine<To> get_return_object() noexcept
		{
			return Coroutine<To>{ std::coroutine_handle<To>::from_promise(*this) };
		}

		void unhandled_exception() const noexcept {}

		using InitialSuspendType = std::
			conditional_t<EagerStart, std::suspend_never, std::suspend_always>;
		InitialSuspendType initial_suspend() const noexcept { return {}; }
		std::suspend_always final_suspend() const noexcept { return {}; }

		template <class T>
		std::suspend_always yield_value(T&&) const noexcept
		{
			return {};
		}
	};
} // namespace apollo::coro