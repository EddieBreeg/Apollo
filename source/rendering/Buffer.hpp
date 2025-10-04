#pragma once

#include <PCH.hpp>

#include "HandleWrapper.hpp"
#include <core/Enum.hpp>

struct SDL_GPUBuffer;
struct SDL_GPUCopyPass;

namespace brk::rdr {
	enum class EBufferFlags : int8
	{
		None = 0,
		Vertex = BIT(0),
		Index = BIT(1),
		GraphicsStorage = BIT(2),
		ComputeStorageRead = BIT(3),
		ComputeStorageWrite = BIT(4),
		NFlags = 4
	};

	class  Buffer : public _internal::HandleWrapper<SDL_GPUBuffer*>
	{
	public:
		using BaseType::BaseType;
		 BRK_API Buffer(EnumFlags<EBufferFlags> usage, uint32 size);
		BRK_API ~Buffer();
		Buffer(Buffer&& other)
			: BaseType(std::move(other))
			, m_Flags(other.m_Flags)
			, m_Size(other.m_Size)
		{
			other.m_Flags = EBufferFlags::None;
			other.m_Size = 0;
		}

		BRK_API void UploadData(
			SDL_GPUCopyPass* copyPass,
			const void* data,
			uint32 size,
			uint32 destOffset = 0);

		void Swap(Buffer& other) noexcept
		{
			BaseType::Swap(other);
			std::swap(m_Flags, other.m_Flags);
			std::swap(m_Size, other.m_Size);
		}

	private:
		EnumFlags<EBufferFlags> m_Flags;
		uint32 m_Size = 0;
	};
} // namespace brk::rdr