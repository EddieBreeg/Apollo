#pragma once

/** \file Memory.hpp */

#include <PCH.hpp>
#include <memory_resource>
#include <type_traits>

namespace apollo {
	/// \brief Generic memory resource, compatible with std::pmr::memory_resource
	/// \details `R` must have 3 public functions:
	/// - `allocate`
	/// - `deallocate`
	/// - `is_equal`
	/// These functions should have the same signature as their std counterparts, but it is not
	/// necessary to actually inherit from std::pmr::memory_resource. See
	/// [std::pmr::memory_resource](https://en.cppreference.com/w/cpp/memory/memory_resource.html)
	/// for more details.
	template <class R>
	concept MemoryResource = requires(
		R & r,
		size_t n,
		size_t a,
		void* ptr,
		const R& lhs,
		const std::pmr::memory_resource& rhs)
	{
		{ r.allocate(n, a) }->std::convertible_to<void*>;
		{ r.deallocate(ptr, n, a) };
		{ lhs.is_equal(rhs) }->std::convertible_to<bool>;
		noexcept(lhs.is_equal(rhs));
	};

	/// \brief Utility class to track memory allocations from a specific source
	/// \tparam ResourceType: The type of memory resource. Has to meet the criteria of
	/// \ref MemoryResource
	template <MemoryResource ResourceType = std::pmr::memory_resource>
	struct AllocationTracker : public std::pmr::memory_resource
	{
		AllocationTracker(ResourceType& upstream, size_t initSize = 0) noexcept
			: m_Upstream(&upstream)
			, m_TotalSize(initSize)
		{}

		/** \name Copy constructor/assignment operator
		 * \brief Copies the underlying resource, but \b not the allocated size
		 * @{ */
		AllocationTracker(const AllocationTracker& other) noexcept
			: m_Upstream(other.m_Upstream)
		{}
		AllocationTracker& operator=(const AllocationTracker& other) noexcept
		{
			m_Upstream = other.m_Upstream;
			return *this;
		}
		/** @} */

		[[nodiscard]] ResourceType* GetResource() const noexcept { return m_Upstream; }
		[[nodiscard]] size_t GetAllocatedSize() const noexcept { return m_TotalSize; }

	private:
		ResourceType* m_Upstream = nullptr;
		size_t m_TotalSize = 0;

		void* do_allocate(size_t size, size_t alignment) override
		{
			m_TotalSize += size;
			return m_Upstream->allocate(size, alignment);
		}
		void do_deallocate(void* ptr, size_t n, size_t alignment) override
		{
			m_TotalSize -= n;
			m_Upstream->deallocate(ptr, n, alignment);
		}
		bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
		{
			if (auto* ptr = dynamic_cast<const AllocationTracker*>(&other))
			{
				return ptr->m_Upstream == m_Upstream;
			}
			return false;
		}
	};

	class MemoryPool;

	template <class T>
	struct PoolAllocator
	{
		using value_type = T;
		using pointer = value_type*;
		using is_always_equal = std::false_type;
		using propagate_on_container_copy_assignment = std::true_type;
		using propagate_on_container_move_assignment = std::true_type;
		using propagate_on_container_swap = std::true_type;

		template <class U>
		friend struct PoolAllocator;

		PoolAllocator(MemoryPool& pool) noexcept;
		PoolAllocator(const PoolAllocator&) noexcept = default;
		PoolAllocator& operator=(const PoolAllocator&) noexcept = default;

		template <class U>
		requires(!std::is_same_v<T, U>) PoolAllocator(const PoolAllocator<U>& other) noexcept;

		template <class U>
		requires(!std::is_same_v<T, U>) PoolAllocator& operator=(
			const PoolAllocator<U>& other) noexcept;

		pointer allocate(size_t n);
		void deallocate(pointer ptr, size_t n);
		void destroy(pointer ptr);

		template <class U = T>
		[[nodiscard]] bool operator==(const PoolAllocator<U>& other) const noexcept;
		template <class U = T>
		[[nodiscard]] bool operator!=(const PoolAllocator<U>& other) const noexcept;

		[[nodiscard]] PoolAllocator<T> select_on_container_copy_construction() noexcept;

		template <class U = T>
		void Swap(PoolAllocator<U> other) noexcept;

	private:
		MemoryPool* m_Pool = nullptr;
	};

	/// \brief Simplistic memory allocation primitive, which allocates memory in fixed size
	/// blocks
	/// \details The pool maintains a linked list of chunks. Each chunk consists of a header,
	/// the actual buffer containing a whole number of blocks and a bit set which tracks the
	/// state of each block.
	/// \warning This resource ignores the alignment operator specified through the `allocate`
	/// function. The returned address will be aligned to a block boundary, so you should make
	/// sure whatever block size you specify includes additional padding to adhere to specific
	/// alignment constraints you may have.
	class MemoryPool : public std::pmr::memory_resource
	{
	public:
		/// \brief Creates a new pool, optionally allocating an empty chunk
		/// \param blockSize: The block size to use for all allocations
		/// \param initialCount: If non 0: the number of blocks to allocate in the first chunk
		/// \param upstreamResource: A memory resource to use when the pool runs out of memory.
		/// Defaults to
		/// [std::pmr::new_delete_resource()](https://en.cppreference.com/w/cpp/memory/new_delete_resource.html).
		APOLLO_API MemoryPool(
			uint32 blockSize,
			uint32 initialCount = 0,
			std::pmr::memory_resource* upstreamResource = nullptr);
		APOLLO_API ~MemoryPool();

		MemoryPool(const MemoryPool&) = delete;
		MemoryPool& operator=(const MemoryPool&) = delete;
		MemoryPool(MemoryPool&& other) noexcept
			: m_UpstreamResource(other.m_UpstreamResource)
			, m_BlockSize(other.m_BlockSize)
			, m_First(other.m_First)
		{
			other.m_UpstreamResource = nullptr;
			other.m_First = nullptr;
			other.m_BlockSize = 0;
		}
		MemoryPool& operator=(MemoryPool&& other) noexcept
		{
			Swap(other);
			return *this;
		};

		void Swap(MemoryPool& other) noexcept
		{
			apollo::Swap(m_UpstreamResource, other.m_UpstreamResource);
			apollo::Swap(m_First, other.m_First);
			apollo::Swap(m_BlockSize, other.m_BlockSize);
		}

		/// \anchor MemoryPool_GetBlockSize
		[[nodiscard]] uint32 GetBlockSize() const noexcept { return m_BlockSize; }
		/// \brief Computes the number of blocks required to hold a specific allocation (in
		/// bytes).
		/// \details This is useful when you need to allocate a specific block of memory and want
		/// to know how many blocks you need. This function is used internally by do_allocate
		/// \returns \code{cpp} (bytes - 1) / m_BlockSize + 1 \endcode
		[[nodiscard]] uint32 ComputeBlockCount(uint32 bytes) const noexcept
		{
			return (bytes - 1) / m_BlockSize + 1;
		}
		/// \brief Computes the provided allocation size, rounded up to the next multiple of
		/// \ref MemoryPool_GetBlockSize "GetBlockSize()"
		/// \returns \code{cpp} m_BlockSize * ComputeBlockCount(bytes) \endcode
		[[nodiscard]] uint32 ComputeAllocationSize(uint32 bytes) const noexcept
		{
			return m_BlockSize * ComputeBlockCount(bytes);
		}

		/// \brief Returns the upstream memory resource responsible for allocating the memory
		/// the pool requires.
		[[nodiscard]] std::pmr::memory_resource& GetUpstreamResource() const noexcept
		{
			return *m_UpstreamResource;
		}

		/// \brief Performs an allocation of n whole blocks
		/// \details First, we try to allocated the blocks from existing chunks - by order of
		/// creation. If no chunk could hold the required number blocks, a new chunk is
		/// allocated big enough to fit at least \p n blocks. The capacity of the new chunk is
		/// based on either that of the previous chunk or \p n - whichever is higher - and is
		/// grown by a factor of 3/2 (rounded up).
		[[nodiscard]] APOLLO_API void* AllocateBlocks(uint32 n);
		/// \brief Deallocates \p n blocks from the pool.
		/// \important The provided pointer must have been allocated from \p this, and \p n must
		/// be at most as big as the original allocation size. This function asserts if the
		/// total area referenced by \p ptr and \p n doesn't belong to the pool.
		APOLLO_API void DeallocateBlocks(void* ptr, uint32 n);

		/// \brief Frees all previously allocated blocks, but doesn't release the allocated
		/// memory back to the upstream resource.
		APOLLO_API void Clear();

		/// \brief Constructs a pool allocator from `this`.
		/// \tparam T: The type of values to allocate. Although `sizeof(T)` is not required to
		/// be a multiple of the block size, this is very much recommended to avoid memory
		/// fragmentation and wasted space.
		template <class T>
		[[nodiscard]] PoolAllocator<T> GetAllocator() noexcept
		{
			return { *this };
		}
		/// \brief Constructs a polymorphic allocator from `this`.
		/// \tparam T: The type of values to allocate. Although `sizeof(T)` is not required to
		/// be a multiple of the block size, this is very much recommended to avoid memory
		/// fragmentation and wasted space.
		template <class T>
		[[nodiscard]] std::pmr::polymorphic_allocator<T> GetPolymorphicAllocator() noexcept
		{
			return { this };
		}

	private:
		/// \brief Called by std::pmr::memory_resource to perform an allocation of \p n bytes.
		/// \note The alignment parameter is ignored: the returns address will be aligned to
		/// block boundary.
		void* do_allocate(size_t n, size_t) override
		{
			return AllocateBlocks(ComputeBlockCount(n));
		}
		/// \brief Called by std::pmr::memory_resource to deallocate \p n bytes
		void do_deallocate(void* ptr, size_t n, size_t) override
		{
			DeallocateBlocks(ptr, ComputeBlockCount(n));
		}
		bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
		{
			return this == &other;
		}

	private:
		struct Chunk;
		Chunk* AllocateChunk(uint32 n, uint32 prealloc = 0);
		Chunk* DeallocateChunk(Chunk*);

		std::pmr::memory_resource* m_UpstreamResource = nullptr;
		uint32 m_BlockSize = 0;
		Chunk* m_First = nullptr;
	};

	// =================================================================

	template <class T>
	PoolAllocator<T>::PoolAllocator(MemoryPool& pool) noexcept
		: m_Pool(&pool)
	{}

	template <class T>
	template <class U>
	requires(!std::is_same_v<T, U>)
		PoolAllocator<T>::PoolAllocator(const PoolAllocator<U>& other) noexcept
		: m_Pool(other.m_Pool)
	{}

	template <class T>
	template <class U>
	requires(!std::is_same_v<T, U>)
		PoolAllocator<T>& PoolAllocator<T>::operator=(const PoolAllocator<U>& other) noexcept
	{
		m_Pool = other.m_Pool;
		return *this;
	}

	template <class T>
	T* PoolAllocator<T>::allocate(size_t n)
	{
		const uint32 count = m_Pool->ComputeBlockCount(n * sizeof(T));
		return static_cast<T*>(m_Pool->AllocateBlocks(count));
	}

	template <class T>
	void PoolAllocator<T>::deallocate(T* ptr, size_t n)
	{
		const uint32 count = m_Pool->ComputeBlockCount(n * sizeof(T));
		m_Pool->DeallocateBlocks(ptr, count);
	}

	template <class T>
	void PoolAllocator<T>::destroy(pointer ptr)
	{
		if constexpr (!std::is_trivially_destructible_v<T>)
			ptr->~T();
	}

	template <class T>
	template <class U>
	[[nodiscard]] bool PoolAllocator<T>::operator==(const PoolAllocator<U>& other) const noexcept
	{
		return m_Pool == other.m_Pool;
	}
	template <class T>
	template <class U>
	[[nodiscard]] bool PoolAllocator<T>::operator!=(const PoolAllocator<U>& other) const noexcept
	{
		return m_Pool != other.m_Pool;
	}

	template <class T>
	[[nodiscard]] PoolAllocator<T> PoolAllocator<T>::select_on_container_copy_construction() noexcept
	{
		return { m_Pool };
	}

	template <class T>
	template <class U>
	void PoolAllocator<T>::Swap(PoolAllocator<U> other) noexcept
	{
		Swap(m_Pool, other.m_Pool);
	}

	template <class T>
	void swap(PoolAllocator<T>& a, PoolAllocator<T>& b) noexcept
	{
		a.Swap(b);
	}
	template <class T, class U>
	void swap(PoolAllocator<T>& a, PoolAllocator<U>& b) noexcept requires(!std::is_same_v<T, U>)
	{
		a.Swap(b);
	}
} // namespace apollo