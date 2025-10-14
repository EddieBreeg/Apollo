#pragma once

#include <PCH.hpp>

namespace apollo {
	/**
	 * Retain action: specifies the reference count should be increment when storing a
	 * new pointer
	 */
	struct Retain_t
	{};
	/**
	 * Retain action: specifies the reference count should be left alone when storing
	 * new pointer
	 */
	struct Adopt_t
	{};

	/**
	 * Retain action. This tells RetainPtr to increment the ref counter when
	 * wrapping a new pointer
	 */
	static constexpr Retain_t s_Retain;
	/**
	 * Adopt action. This tells RetainPtr to leave the ref counter alone when
	 * wrapping a new pointer
	 */
	static constexpr Adopt_t s_Adopt;

	/**
	 * Trait class which implements the necessary APIs for a given ref counter class
	 * \tparam T: The ref counted class
	 * \details A valid retain traits class must have at least 3 static functions:
	 * Increment, Decrement and GetCount. Additionally, it can also provide a static
	 * DefaultAction value, to specify which should be the default behaviour when creating
	 * a new RetainPtr. If not specified, s_Adopt will be used.
	 * It must also declare a public static const value named DefaultAction of type either
	 * Retain_t or Adopt_t. This value specifies the default behaviour when storing a new
	 * pointer in a RetainPtr object
	 */
	// template <class T>
	// struct RetainTraits;

	template <class TraitType, class T>
	concept RetainTraits = requires(T * ptr)
	{
		{ TraitType::Increment(ptr) };
		{ TraitType::Decrement(ptr) };
		{ TraitType::GetCount((const T*)ptr) }->std::convertible_to<uint32>;
	};

	/**
	 * Default retain traits. RetainPtr will default to this unless otherwise specified
	 */
	template <class T>
	struct DefaultRetainTraits
	{
		static constexpr Retain_t DefaultAction{};
		static void Increment(T* ptr) noexcept { ++ptr->m_RefCount; }
		static void Decrement(T* ptr) noexcept
		{
			if (!--ptr->m_RefCount)
				delete ptr;
		}
		static uint32 GetCount(const T* ptr) noexcept { return ptr->m_RefCount; }
	};

	/**
	 *  A smart pointer with intrusive reference counting
	 * \tparam T: The type of object pointed at
	 * \tparam Traits: The retain traits which defines all relevant operations on the
	 * reference counter (Increment, Decrement and GetCount) as well as DefaultAction, which informs
	 * the default behaviour when wrapping a pointer. See Adopt_t and Retain_t for details
	 */
	template <class T, RetainTraits<T> Traits = DefaultRetainTraits<T>>
	class RetainPtr
	{
	public:
		using TraitsType = Traits;
		using CounterType = decltype(TraitsType::GetCount(std::declval<const T*>()));

		/**
		 * Initializes the stored pointer to nullptr
		 */
		constexpr RetainPtr() = default;
		/**
		 * Constructs an object which stores the specified pointer, without incrementing
		 * the reference count
		 */
		RetainPtr(T* ptr, Adopt_t);
		/**
		 * Stores the specified pointer, and increments its reference count if ptr is not
		 * nullptr
		 */
		RetainPtr(T* ptr, Retain_t);
		/**
		 * Stores the specified pointer, and derives the ref count behaviour from
		 * Traits::DefaultAction
		 */
		explicit RetainPtr(T* ptr);

		/** Copies the stored pointer and increments the ref count if not null */
		RetainPtr(const RetainPtr& other);
		RetainPtr(RetainPtr&& other) noexcept;

		/** Effectively calls Reset(ptr, other.ptr) */
		RetainPtr& operator=(const RetainPtr& other);
		/** Effectively calls std::swap(ptr, other.ptr) */
		RetainPtr& operator=(RetainPtr&& other) noexcept;

		/** If the stored pointer is not null, decrements the reference counter */
		~RetainPtr();

		/** Replaces the stored pointer with the specified value, without incrementing the
		 * reference counter. If the previously stored pointer was not nullptr, the
		 * reference count is decremented */
		void Reset(T* ptr, Adopt_t);
		/** Replaces the stored pointer with the specified value, and increments the
		 * reference counter if said value is not nullptr. . If the previously stored
		 * pointer was not nullptr, the reference count is decremented */
		void Reset(T* ptr, Retain_t);
		/** Calls Reset(ptr, Traits::DefaultAction) */
		void Reset(T* ptr = nullptr);

		[[nodiscard]] T* Get() noexcept { return m_Ptr; }
		[[nodiscard]] const T* Get() const noexcept { return m_Ptr; }
		T* operator->() noexcept { return m_Ptr; }
		const T* operator->() const noexcept { return m_Ptr; }
		[[nodiscard]] T& operator*() { return *m_Ptr; }
		[[nodiscard]] const T& operator*() const { return *m_Ptr; }

		[[nodiscard]] constexpr operator bool() const noexcept { return bool(m_Ptr); }

		/** Returns the current value of the reference counter. Only thread safe is the
		 * counter itself is atomic */
		[[nodiscard]] CounterType GetCount() const;

		/** Replaces the stored pointer with nullptr without decrementing the reference
		 * count, and returns the old value */
		T* Release() noexcept { return std::exchange(m_Ptr, nullptr); }

		template <class U = T, class UTraits = TraitsType>
		[[nodiscard]] constexpr bool operator==(const RetainPtr<U, UTraits>& other) const noexcept
		{
			return m_Ptr == other.m_Ptr;
		}

		template <class U = T, class UTraits = TraitsType>
		[[nodiscard]] constexpr bool operator!=(const RetainPtr<U, UTraits>& other) const noexcept
		{
			return m_Ptr != other.m_Ptr;
		}

		void Swap(RetainPtr& other) noexcept { std::swap(m_Ptr, other.m_Ptr); }

	private:
		T* m_Ptr = nullptr;
	};

	template <class To, class From, class Traits>
	RetainPtr<To, Traits> StaticPointerCast(RetainPtr<From, Traits> ptr) noexcept
	{
		return RetainPtr<To, Traits>{ static_cast<To*>(ptr.Release()), s_Adopt };
	}

	template <class To, class From, class Traits>
	RetainPtr<To, Traits> DynamicPointerCast(RetainPtr<From, Traits> ptr) noexcept
	{
		return RetainPtr<To, Traits>{ dynamic_cast<To*>(ptr.Release()), s_Adopt };
	}

	template <class T, RetainTraits<T> Traits>
	inline RetainPtr<T, Traits>::RetainPtr(T* ptr, Adopt_t)
		: m_Ptr{ ptr }
	{}

	template <class T, RetainTraits<T> Traits>
	inline RetainPtr<T, Traits>::RetainPtr(T* ptr, Retain_t)
		: m_Ptr{ ptr }
	{
		if (m_Ptr)
			Traits::Increment(m_Ptr);
	}

	template <class T, RetainTraits<T> Traits>
	inline RetainPtr<T, Traits>::RetainPtr(T* ptr)
		: RetainPtr{ ptr, Traits::DefaultAction }
	{}

	template <class T, RetainTraits<T> Traits>
	inline RetainPtr<T, Traits>::~RetainPtr()
	{
		Reset();
	}

	template <class T, RetainTraits<T> Traits>
	inline RetainPtr<T, Traits>::RetainPtr(const RetainPtr& other)
		: m_Ptr{ other.m_Ptr }
	{
		if (m_Ptr)
			Traits::Increment(m_Ptr);
	}

	template <class T, RetainTraits<T> Traits>
	inline RetainPtr<T, Traits>::RetainPtr(RetainPtr&& other) noexcept
		: m_Ptr{ other.m_Ptr }
	{
		other.m_Ptr = nullptr;
	}

	template <class T, RetainTraits<T> Traits>
	inline RetainPtr<T, Traits>& RetainPtr<T, Traits>::operator=(const RetainPtr& other)
	{
		Reset(other.m_Ptr, s_Retain);
		return *this;
	}

	template <class T, RetainTraits<T> Traits>
	inline RetainPtr<T, Traits>& RetainPtr<T, Traits>::operator=(RetainPtr&& other) noexcept
	{
		std::swap(m_Ptr, other.m_Ptr);
		return *this;
	}

	template <class T, RetainTraits<T> Traits>
	inline void RetainPtr<T, Traits>::Reset(T* ptr, Adopt_t)
	{
		if (m_Ptr)
			Traits::Decrement(m_Ptr);
		m_Ptr = ptr;
	}

	template <class T, RetainTraits<T> Traits>
	inline void RetainPtr<T, Traits>::Reset(T* ptr, Retain_t)
	{
		if (m_Ptr)
			Traits::Decrement(m_Ptr);
		if ((m_Ptr = ptr))
			Traits::Increment(m_Ptr);
	}

	template <class T, RetainTraits<T> Traits>
	inline void RetainPtr<T, Traits>::Reset(T* ptr)
	{
		Reset(ptr, Traits::DefaultAction);
	}

	template <class T, RetainTraits<T> Traits>
	inline RetainPtr<T, Traits>::CounterType apollo::RetainPtr<T, Traits>::GetCount() const
	{
		using TRes = decltype(Traits::GetCount(m_Ptr));
		if (!m_Ptr)
			return TRes{ 0 };
		return Traits::GetCount(m_Ptr);
	}
} // namespace apollo