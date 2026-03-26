#pragma once

#include <PCH.hpp>

/** \file RetainPtr.hpp
 * \brief RetainPtr API

 This API serves as a base to implement smart pointers with intrusive reference counting. Custom
 behaviour for a reference counted type is implemented using a traits type, which must meet the
 criteria of the apollo::RetainTraits concept.
*/

namespace apollo {
	/**
	 * \brief Retain action type: specifies the reference count should be incremented by default
	 * when storing a new pointer
	 * \sa RetainPtr
	 */
	struct Retain_t
	{};
	/**
	 * \brief Retain action type: specifies the reference count should be left alone when storing a
	 * new pointer
	 * \sa RetainPtr
	 */
	struct Adopt_t
	{};

	/**
	 * \brief Retain action. This tells RetainPtr to increment the ref counter when
	 * wrapping a new pointer
	 */
	static constexpr Retain_t s_Retain;
	/**
	 * \brief Adopt action. This tells RetainPtr to leave the ref counter alone when
	 * wrapping a new pointer
	 */
	static constexpr Adopt_t s_Adopt;

	/**
	 * \brief Trait class which implements the necessary APIs for a given ref counter class
	 * \tparam T: The ref counted class
	 * \details A valid retain traits class must have at least 3 static functions:
	 * Increment, Decrement and GetCount.
	 * It must also declare a public static const value named DefaultAction of type either
	 * Retain_t or Adopt_t. This value specifies the default behaviour when storing a new
	 * pointer in a RetainPtr object
	 */
	template <class TraitType, class T>
	concept RetainTraits = requires(T * ptr)
	{
		{ TraitType::Increment(ptr) };
		{ TraitType::Decrement(ptr) };
		{ TraitType::GetCount((const T*)ptr) }->std::convertible_to<uint32>;
		{ std::is_same_v<std::remove_cv_t<decltype(TraitType::DefaultAction)>, Retain_t> ||
		  std::is_same_v<std::remove_cv_t<decltype(TraitType::DefaultAction)>, Adopt_t> };
	};

	/**
	 * \brief Default retain traits. RetainPtr will default to this unless otherwise specified.
	 * \tparam T: The ref counted object type
	 * \details This implementation assumes \p T declares its reference count as a member variable
	 * named <i>m_RefCount</i>. When the reference count reaches 0, \b delete is used to destroy the
	 * object and release the memory.
	 */
	template <class T>
	struct DefaultRetainTraits
	{
		static constexpr Retain_t DefaultAction{};
		static void Increment(T* ptr) noexcept { ++ptr->m_RefCount; }
		static void Decrement(T* ptr)
		{
			if (!--ptr->m_RefCount)
				delete ptr;
		}
		static uint32 GetCount(const T* ptr) noexcept { return ptr->m_RefCount; }
	};

	/**
	 * \brief A smart pointer with intrusive reference counting
	 * \tparam T: The reference counted object type
	 * \tparam Traits: The retain traits which defines all relevant operations on the
	 * reference counter (Increment, Decrement and GetCount) as well as DefaultAction, which informs
	 * the default behaviour when wrapping a pointer.
	 * \sa RetainTraits
	 * \sa Adopt_t
	 * \sa Retain_t
	 */
	template <class T, RetainTraits<T> Traits = DefaultRetainTraits<T>>
	class RetainPtr
	{
	public:
		using TraitsType = Traits;
		using CounterType = decltype(TraitsType::GetCount(std::declval<const T*>()));

		/**
		 * \brief Initializes the stored pointer to nullptr
		 */
		constexpr RetainPtr() = default;
		/**
		 * \brief Constructs an object which stores the specified pointer, without incrementing
		 * the reference count
		 */
		RetainPtr(T* ptr, Adopt_t);
		/**
		 * \brief Stores the specified pointer, and increments its reference count if ptr is not
		 * nullptr
		 */
		RetainPtr(T* ptr, Retain_t);
		/**
		 * \brief Stores the specified pointer, and derives the ref count behaviour from
		 * Traits::DefaultAction
		 */
		explicit RetainPtr(T* ptr);

		/** \brief Copies the stored pointer and increments the ref count if not null */
		RetainPtr(const RetainPtr& other);
		/** \brief Moves a RetainPtr object into *this. After this call, \p other.Get() returns \b
		 * nullptr */
		RetainPtr(RetainPtr&& other) noexcept;

		/** \brief Effectively calls Reset(ptr, other.ptr) */
		RetainPtr& operator=(const RetainPtr& other);
		/** \brief Effectively calls std::swap(ptr, other.ptr) */
		RetainPtr& operator=(RetainPtr&& other) noexcept;

		/** \brief If the stored pointer is not null, decrements the reference count */
		~RetainPtr();

		/** \brief Replaces the stored pointer.
		 * \param ptr: The new value to store
		 * \details \p ptr is left unchanged (its reference count is not increment). If the old
		 * stored value was not <b>nullptr</b>, its reference count is decremented before the new
		 * value is assigned.
		 */
		void Reset(T* ptr, Adopt_t);
		/** \brief Replaces the stored pointer.
		 * \param ptr: The new value to store
		 * \details If \p ptr is not nullptr, its reference count is incremented. If the old
		 * stored value was not <b>nullptr</b>, its reference count is decremented before the new
		 * value is assigned.
		 */
		void Reset(T* ptr, Retain_t);
		/** \brief Calls Reset(ptr, Traits::DefaultAction) */
		void Reset(T* ptr = nullptr);

		/** \name Accessor functions
		 * @{ */
		[[nodiscard]] T* Get() noexcept { return m_Ptr; }
		[[nodiscard]] const T* Get() const noexcept { return m_Ptr; }
		T* operator->() noexcept { return m_Ptr; }
		const T* operator->() const noexcept { return m_Ptr; }
		[[nodiscard]] T& operator*() { return *m_Ptr; }
		[[nodiscard]] const T& operator*() const { return *m_Ptr; }
		/** @} */

		/** \brief Converts the stored value to \b bool */
		[[nodiscard]] constexpr operator bool() const noexcept { return bool(m_Ptr); }

		/** \brief Returns the current value of the reference counter. Only thread safe is the
		 * counter itself is atomic.
		 * \warning The behaviour is undefined if the stored value is \b nullptr
		 */
		[[nodiscard]] CounterType GetCount() const;

		/** \brief Replaces the stored pointer with nullptr without decrementing the reference
		 * count, and returns the old value
		 * \returns \code std::exchange(m_Ptr, nullptr) \endcode
		 */
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

	/** \name Cast functions
	 * \brief Casts a RetainPtr from one object type to another
	 * @{ */

	/** \brief static_cast conversion
	 * \returns \code RetainPtr<To, Traits>{ static_cast<To*>(ptr.Release()), s_Adopt } \endcode */
	template <class To, class From, class Traits>
	RetainPtr<To, Traits> StaticPointerCast(RetainPtr<From, Traits> ptr) noexcept
	{
		return RetainPtr<To, Traits>{ static_cast<To*>(ptr.Release()), s_Adopt };
	}

	/**
	 * \brief dynamic_cast conversion
	 * \details Attempts to convert the pointer using a dynamic cast expression. If the conversion
	 * fails (i.e. the conversion was not type safe), the returned object will contain \b nullptr.
	 * \returns \code RetainPtr<To, Traits>{ dynamic_cast<To*>(ptr.Release()), s_Adopt } \endcode
	 */
	template <class To, class From, class Traits>
	RetainPtr<To, Traits> DynamicPointerCast(RetainPtr<From, Traits> ptr) noexcept
	{
		return RetainPtr<To, Traits>{ dynamic_cast<To*>(ptr.Release()), s_Adopt };
	}
	/** @} */

	// Implementation below

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