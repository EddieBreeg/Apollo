#include "Memory.hpp"
#include <core/Assert.hpp>
#include <core/Bit.hpp>
#include <memory_resource>

namespace apollo {
	struct MemoryPool::Chunk
	{
		Chunk* m_Next = nullptr;
		uint32 m_Capacity = 0;
		uint32 m_FirstAvailable = 0;
		uint32 m_ByteSize = 0;

		void* GetBuffer() noexcept { return this + 1; }
		[[nodiscard]] uint64* GetBits(PointerDiff pos = 0)
		{
			pos.m_Value += m_ByteSize;
			return static_cast<uint64*>(GetBuffer() + pos);
		}

		void* TryAllocateBlocks(uint32 n, uint32 blockSize)
		{
			if (n > (m_Capacity - m_FirstAvailable))
				return nullptr;

			uint64* const state = GetBits();
			BitIterator pos{ state + m_FirstAvailable / 64, m_FirstAvailable % 64 };
			BitIterator end{ state + m_Capacity / 64, m_Capacity % 64 };
			uint32 runLength = 0;
			uint32 offset = m_FirstAvailable;

			for (auto it = pos; it != end; ++it)
			{
				++runLength;
				if (*it)
				{
					++(pos = it);
					offset += runLength;
					runLength = 0;
					continue;
				}
				else if (runLength == n)
					goto DO_ALLOCATE;
			}
			return nullptr;

		DO_ALLOCATE:
			if (offset == m_FirstAvailable)
			{
				m_FirstAvailable += n;
			}
			while (n--)
			{
				pos.Set()++;
			}
			return GetBuffer() + PointerDiff{ offset * blockSize };
		}
		bool TryDeallocate(void* ptr, uint32 n, uint32 blockSize)
		{
			if (n > m_Capacity)
				return false;

			const uint32 offset = (static_cast<std::byte*>(ptr) -
								   static_cast<std::byte*>(GetBuffer())) /
								  blockSize;
			if (offset >= m_Capacity)
				return false;

			BitIterator it{ GetBits() + offset / 64, offset % 64 };
			m_FirstAvailable = Min(offset, m_FirstAvailable);
			while (n--)
			{
				APOLLO_ASSERT(*it, "Block at address {} was already free", ptr);
				it.Clear()++;
			}

			return true;
		}

		void Clear()
		{
			m_FirstAvailable = 0;
			uint64* it = GetBits();
			uint64* end = it + (m_Capacity - 1) / 64 + 1;
			while (it != end)
				*it++ = 0;
		}
	};

	MemoryPool::~MemoryPool()
	{
		while (m_First)
		{
			m_First = DeallocateChunk(m_First);
		}
	}

	MemoryPool::MemoryPool(
		uint32 blockSize,
		uint32 initialAllocSize,
		std::pmr::memory_resource* upstreamResource)
		: m_UpstreamResource(upstreamResource ? upstreamResource : std::pmr::new_delete_resource())
		, m_BlockSize(blockSize)
	{
		APOLLO_ASSERT(blockSize, "Passed 0 as the block size to MemoryPool");
		if (!initialAllocSize)
			return;

		m_First = AllocateChunk(initialAllocSize);
	}

	void* MemoryPool::AllocateBlocks(uint32 n)
	{
		if (!m_First)
		{
			const uint32 capacity = (3 * n - 1) / 2 + 1;
			m_First = AllocateChunk(capacity, n);
			return m_First->GetBuffer();
		}

		Chunk* last = nullptr;
		for (Chunk* it = m_First; it; it = it->m_Next)
		{
			if (void* p = it->TryAllocateBlocks(n, m_BlockSize))
				return p;
			last = it;
		}
		const uint32 capacity = (3 * Max(n, last->m_Capacity) - 1) / 2 + 1;
		Chunk* chunk = last->m_Next = AllocateChunk(capacity, n);
		return chunk->GetBuffer();
	}

	void MemoryPool::DeallocateBlocks(void* ptr, uint32 n)
	{
		for (auto* it = m_First; it; it = it->m_Next)
		{
			if (it->TryDeallocate(ptr, n, m_BlockSize))
				return;
		}
		APOLLO_LOG_CRITICAL("Failed to deallocate {} blocks from address {}", n, ptr);
	}

	MemoryPool::Chunk* MemoryPool::AllocateChunk(uint32 n, uint32 prealloc)
	{
		const uint32 bitsetSize = ((n - 1) / 64 + 1);
		const uint32 chunkSize = Align(n * m_BlockSize, alignof(uint64));
		n = chunkSize / m_BlockSize; // re-compute block count to account for padding

		const PointerDiff bitsetOffset = sizeof(Chunk) + chunkSize;
		const uint32 allocSize = bitsetOffset + bitsetSize * 8;

		void* ptr = m_UpstreamResource->allocate(allocSize, alignof(Chunk));

		Chunk* chunk = new (ptr) Chunk{
			.m_Capacity = n,
			.m_FirstAvailable = prealloc,
			.m_ByteSize = chunkSize,
		};
		uint64* it = static_cast<uint64*>(ptr + bitsetOffset);
		uint64* const end = it + bitsetSize;

		while (prealloc >= 64)
		{
			*it++ = ~0ull;
			prealloc -= 64;
		}
		if (const uint64 tail = (1ull << prealloc) - 1)
		{
			*it++ = tail;
		}
		while (it != end)
		{
			*it++ = 0;
		}
		return chunk;
	}

	MemoryPool::Chunk* MemoryPool::DeallocateChunk(Chunk* ptr)
	{
		Chunk* const next = ptr->m_Next;
		const uint32 bitsetSize = (ptr->m_Capacity - 1) / 64 + 1;
		m_UpstreamResource->deallocate(
			ptr,
			sizeof(Chunk) + ptr->m_ByteSize + 8 * bitsetSize,
			alignof(Chunk));

		return next;
	}

	void MemoryPool::Clear()
	{
		for (auto it = m_First; it; it = it->m_Next)
			it->Clear();
	}
} // namespace apollo

#undef VOID_PTR_ADD