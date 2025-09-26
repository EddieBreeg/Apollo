#pragma once

#include <PCH.hpp>

#include "TypeTraits.hpp"

#include <memory>
#include <stdexcept>

namespace brk {
	template <class T, class Alloc = std::allocator<T>>
	struct Queue
	{
	public:
		using AllocatorTraits = std::allocator_traits<Alloc>;

		constexpr Queue() = default;

		template <class A = Alloc>
		explicit Queue(A&& alloc) requires(std::is_constructible_v<Alloc, A>);

		template <class A = Alloc>
		explicit Queue(uint32 capacity, A&& alloc = {}) requires(std::is_constructible_v<Alloc, A>);

		~Queue();

		Queue(const Queue& other);
		Queue(Queue&& other) noexcept;

		Queue& operator=(const Queue& other);
		Queue& operator=(Queue&& other) noexcept(
			(AllocatorTraits::propagate_on_container_move_assignment::value &&
			 std::is_nothrow_swappable_v<Alloc>) ||
			AllocatorTraits::is_always_equal::value);

		[[nodiscard]] uint32 GetSize() const noexcept { return m_Size; }
		[[nodiscard]] uint32 GetCapacity() const noexcept { return m_Capacity; }

		template <class U = T>
		void Add(U&& val) requires std::constructible_from<T, U>;

		template <class... A>
		void AddEmplace(A&&... args) requires std::is_constructible_v<T, A...>;

		void PopFront() noexcept(std::is_nothrow_destructible_v<T>);
		[[nodiscard]] T PopAndGetFront() requires(meta::NoThrowMovable<T>);

		T& GetFront();
		const T& GetFront() const;

		Alloc& GetAllocator() noexcept { return m_Allocator; }
		const Alloc& GetAllocator() const noexcept { return m_Allocator; }

		void Clear();
		void Reserve(uint32 capacity);

		[[nodiscard]] T& Get(uint32 i);
		[[nodiscard]] const T& Get(uint32 i) const;

		[[nodiscard]] T& operator[](uint32 i) { return Get(i); }
		[[nodiscard]] const T& operator[](uint32 i) const { return Get(i); }

		void Swap(Queue& other) noexcept(
			!AllocatorTraits::propagate_on_container_swap::value ||
			std::is_nothrow_swappable_v<Alloc>)
			requires(
				AllocatorTraits::propagate_on_container_swap::value ||
				AllocatorTraits::is_always_equal::value);

	private:
		[[nodiscard]] static uint32 GrowCapacity(uint32 n);
		void Reallocate(uint32 newCap);

		void PropagateAllocator(const Alloc& alloc);

		T* m_Ptr = nullptr;

		uint32 m_Start = 0;
		uint32 m_Size = 0;
		uint32 m_Capacity = 0;
		Alloc m_Allocator;
	};

	/**
	 * Queue Implementation
	 * ====================
	 */

	template <class T, class Alloc>
	template <class A>
	Queue<T, Alloc>::Queue(A&& alloc) requires(std::is_constructible_v<Alloc, A>)
		: m_Allocator(std::forward<A>(alloc))
	{}

	template <class T, class Alloc>
	template <class A>
	Queue<T, Alloc>::Queue(uint32 cap, A&& alloc) requires(std::is_constructible_v<Alloc, A>)
		: m_Capacity(cap)
		, m_Allocator(std::forward<A>(alloc))
	{
		m_Ptr = m_Allocator.allocate(m_Capacity);
	}

	template <class T, class Alloc>
	uint32 Queue<T, Alloc>::GrowCapacity(uint32 n)
	{
		if (!n)
			return 1;
		return (3 * n - 1) / 2 + 1;
	}

	template <class T, class A>
	void Queue<T, A>::Reallocate(uint32 newCap)
	{
		T* dest = m_Allocator.allocate(newCap);
		if (!m_Ptr)
			goto REALLOC_END;

		for (uint32 i = 0; i < m_Size; ++i)
			new (dest + i) T{ std::move(Get(i)) };
		m_Start = 0;
		m_Allocator.deallocate(m_Ptr, m_Capacity);

	REALLOC_END:
		m_Capacity = newCap;
		m_Ptr = dest;
	}

	template <class T, class Alloc>
	void Queue<T, Alloc>::Clear()
	{
		if (!m_Size)
			return;

		if constexpr (!std::is_trivially_destructible_v<T>)
		{
			for (uint32 i = 0; i < m_Size; ++i)
			{
				const uint32 pos = (m_Start + i) % m_Capacity;
				std::destroy_at(m_Ptr + pos);
			}
		}
		m_Size = 0;
	}

	template <class T, class Alloc>
	Queue<T, Alloc>::~Queue()
	{
		if (m_Ptr)
		{
			Clear();
			m_Allocator.deallocate(m_Ptr, m_Capacity);
		}
	}

	template <class T, class A>
	Queue<T, A>::Queue(const Queue& other)
		: m_Size(other.m_Size)
		, m_Capacity(other.m_Capacity)
		, m_Allocator(other.m_Allocator)
	{
		m_Ptr = m_Allocator.allocate(m_Capacity);
		for (uint32 i = 0; i < m_Size; ++i)
		{
			new (m_Ptr + i) T{ other.Get(i) };
		}
	}
	template <class T, class A>
	Queue<T, A>::Queue(Queue&& other) noexcept
		: m_Ptr(other.m_Ptr)
		, m_Start(other.m_Start)
		, m_Size(other.m_Size)
		, m_Capacity(other.m_Capacity)
		, m_Allocator(std::move(other.m_Allocator))
	{
		other.m_Ptr = nullptr;
		other.m_Size = other.m_Capacity = other.m_Start = 0;
	}

	template <class T, class A>
	Queue<T, A>& Queue<T, A>::operator=(const Queue& other)
	{
		using AllocTraits = std::allocator_traits<A>;
		if constexpr (AllocTraits::propagate_on_container_copy_assignment::value)
		{
			PropagateAllocator(other.m_Allocator);
		}
		if (other.m_Size > m_Capacity)
		{
			Clear();
			Reallocate(other.m_Capacity);

			while (m_Size < other.m_Size)
			{
				new (m_Ptr + m_Size) T{ other.Get(m_Size) };
				++m_Size;
			}
			return *this;
		}

		for (uint32 i = 0; i < m_Size && i < other.m_Size; ++i)
		{
			Get(i) = other.Get(i);
		}
		while (m_Size < other.m_Size)
		{
			const uint32 i = (m_Start + m_Size) % m_Capacity;
			new (m_Ptr + i) T{ other.Get(m_Size++) };
		}
		if constexpr (!std::is_trivially_destructible_v<T>)
		{
			while (other.m_Size < m_Size)
			{
				Get(--m_Size).~T();
			}
		}
		else
		{
			m_Size = other.m_Size;
		}

		return *this;
	}

	template <class T, class Alloc>
	Queue<T, Alloc>& Queue<T, Alloc>::operator=(Queue&& other) noexcept(
		(std::allocator_traits<Alloc>::propagate_on_container_move_assignment::value &&
		 std::is_nothrow_swappable_v<Alloc>) ||
		std::allocator_traits<Alloc>::is_always_equal::value)
	{
		using Traits = std::allocator_traits<Alloc>;
		if constexpr (Traits::propagate_on_container_move_assignment::value)
		{
			std::swap(m_Allocator, other.m_Allocator);
			std::swap(m_Ptr, other.m_Ptr);
			std::swap(m_Start, other.m_Start);
			std::swap(m_Size, other.m_Size);
			std::swap(m_Capacity, other.m_Capacity);
			return *this;
		}
		else if (Traits::is_always_equal::value || m_Allocator == other.m_Allocator)
		{
			std::swap(m_Ptr, other.m_Ptr);
			std::swap(m_Start, other.m_Start);
			std::swap(m_Size, other.m_Size);
			std::swap(m_Capacity, other.m_Capacity);
			return *this;
		}
		else if (m_Capacity < other.m_Size)
		{
			Clear();
			Reallocate(other.m_Capacity);
			while (m_Size < other.m_Size)
			{
				new (m_Ptr + m_Size) T{ std::move(other.Get(m_Size)) };
				++m_Size;
			}
			return *this;
		}

		for (uint32 i = 0; i < m_Size && i < other.m_Size; ++i)
		{
			Get(i) = std::move(other.Get(i));
		}
		while (m_Size < other.m_Size)
		{
			const uint32 i = (m_Start + m_Size) % m_Capacity;
			new (m_Ptr + i) T{ std::move(other.Get(m_Size++)) };
		}
		if constexpr (!std::is_trivially_destructible_v<T>)
		{
			while (other.m_Size < m_Size)
			{
				Get(--m_Size).~T();
			}
		}
		else
		{
			m_Size = other.m_Size;
		}

		return *this;
	}

	template <class T, class Alloc>
	template <class U>
	void Queue<T, Alloc>::Add(U&& val) requires std::constructible_from<T, U>
	{
		if (m_Size == m_Capacity)
		{
			Reallocate(GrowCapacity(m_Capacity));
		}
		const uint32 index = (m_Start + m_Size++) % m_Capacity;
		new (m_Ptr + index) T{ std::forward<U>(val) };
	}

	template <class T, class Alloc>
	template <class... A>
	void Queue<T, Alloc>::AddEmplace(A&&... args) requires std::is_constructible_v<T, A...>
	{
		if (m_Size == m_Capacity)
		{
			Reallocate(GrowCapacity(m_Capacity));
		}
		const uint32 index = (m_Start + m_Size++) % m_Capacity;
		new (m_Ptr + index) T{ std::forward<A>(args)... };
	}

	template <class T, class Alloc>
	T& Queue<T, Alloc>::GetFront()
	{
		if (!m_Size) [[unlikely]]
		{
			throw std::runtime_error("Called GetFront on empty Queue");
		}
		return m_Ptr[m_Start];
	}

	template <class T, class Alloc>
	const T& Queue<T, Alloc>::GetFront() const
	{
		if (!m_Size) [[unlikely]]
		{
			throw std::runtime_error("Called GetFront on empty Queue");
		}
		return m_Ptr[m_Start];
	}

	template <class T, class Alloc>
	T Queue<T, Alloc>::PopAndGetFront() requires(meta::NoThrowMovable<T>)
	{
		if (!m_Size) [[unlikely]]
		{
			throw std::runtime_error("Called PopAndGetFront on empty Queue");
		}

		T* const ptr = m_Ptr + m_Start;
		if (!--m_Size)
			m_Start = 0;
		else
			m_Start = (m_Start + 1) % m_Capacity;
		return std::move(*ptr);
	}

	template <class T, class Alloc>
	void Queue<T, Alloc>::PopFront() noexcept(std::is_nothrow_destructible_v<T>)
	{
		if (!m_Size) [[unlikely]]
			return;

		if constexpr (!std::is_trivially_destructible_v<T>)
			std::destroy_at(m_Ptr + m_Start);

		if (!--m_Size)
			m_Start = 0;
		else
			m_Start = (m_Start + 1) % m_Capacity;
	}

	template <class T, class A>
	void Queue<T, A>::Reserve(uint32 cap)
	{
		if (cap > m_Capacity)
			Reallocate(cap);
	}

	template <class T, class A>
	T& Queue<T, A>::Get(uint32 i)
	{
		if (i >= m_Size) [[unlikely]]
		{
			throw std::out_of_range("Index out of range");
		}
		return m_Ptr[(m_Start + i) % m_Capacity];
	}

	template <class T, class A>
	const T& Queue<T, A>::Get(uint32 i) const
	{
		if (i >= m_Size) [[unlikely]]
		{
			throw std::out_of_range("Index out of range");
		}
		return m_Ptr[(m_Start + i) % m_Capacity];
	}

	template <class T, class Alloc>
	void Queue<T, Alloc>::PropagateAllocator(const Alloc& alloc)
	{
		using Traits = std::allocator_traits<Alloc>;
		if (!Traits::is_always_equal::value && m_Allocator != alloc)
		{
			Clear();
			m_Allocator.deallocate(m_Ptr, m_Capacity);
			m_Ptr = nullptr;
			m_Capacity = 0;
		}
	}

	template <class T, class Alloc>
	void Queue<T, Alloc>::Swap(Queue& other) noexcept(
		!AllocatorTraits::propagate_on_container_swap::value || std::is_nothrow_swappable_v<Alloc>)
		requires(
			AllocatorTraits::propagate_on_container_swap::value ||
			AllocatorTraits::is_always_equal::value)

	{
		using Traits = std::allocator_traits<Alloc>;
		if constexpr (Traits::propagate_on_container_swap::value)
		{
			std::swap(m_Allocator, other.m_Allocator);
		}
		std::swap(m_Ptr, other.m_Ptr);
		std::swap(m_Start, other.m_Start);
		std::swap(m_Size, other.m_Size);
		std::swap(m_Capacity, other.m_Capacity);
		return;
	}

} // namespace brk