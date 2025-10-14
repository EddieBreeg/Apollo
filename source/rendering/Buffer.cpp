#include "Buffer.hpp"
#include "Renderer.hpp"
#include <SDL3/SDL_gpu.h>
#include <core/Assert.hpp>

namespace apollo::rdr {
	Buffer::Buffer(EnumFlags<EBufferFlags> usage, uint32 size)
		: m_Flags(usage)
		, m_Size(size)
	{
		APOLLO_ASSERT(m_Flags.m_Value, "At least one buffer usage flag must be set");

		static constexpr EBufferFlags vertexIndexBits = EBufferFlags::Vertex | EBufferFlags::Index;
		static constexpr EBufferFlags storageBits = EBufferFlags::GraphicsStorage |
													EBufferFlags::ComputeStorageRead |
													EBufferFlags::ComputeStorageWrite;
		DEBUG_CHECK(!m_Flags.HasAll(vertexIndexBits))
		{
			APOLLO_LOG_ERROR("EBufferFlags::Vertex and EBufferFlags::Index are incompatible");
			m_Flags = EBufferFlags::None;
			return;
		}
		DEBUG_CHECK(!(m_Flags.HasAny(storageBits) && m_Flags.HasAny(vertexIndexBits)))
		{
			APOLLO_LOG_ERROR(
				"EBufferFlags::Vertex/EBufferFlags::Index buffer flags are incompatible with "
				"EBufferFlags::GraphicsStorage/EBufferFlags::ComputeStorage");
		}

		APOLLO_ASSERT(m_Size, "Buffer size may not be 0");

		SDL_GPUBufferCreateInfo info{ .size = m_Size };
		info.usage |= m_Flags.HasAny(EBufferFlags::Vertex) * SDL_GPU_BUFFERUSAGE_VERTEX;
		info.usage |= m_Flags.HasAny(EBufferFlags::Index) * SDL_GPU_BUFFERUSAGE_INDEX;
		info.usage |= m_Flags.HasAny(EBufferFlags::GraphicsStorage) *
					  SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
		info.usage |= m_Flags.HasAny(EBufferFlags::ComputeStorageRead) *
					  SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
		info.usage |= m_Flags.HasAny(EBufferFlags::ComputeStorageWrite) *
					  SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
		m_Handle = SDL_CreateGPUBuffer(Renderer::GetInstance()->GetDevice().GetHandle(), &info);
	}

	Buffer::~Buffer()
	{
		if (m_Handle)
			SDL_ReleaseGPUBuffer(Renderer::GetInstance()->GetDevice().GetHandle(), m_Handle);
	}

	void Buffer::UploadData(SDL_GPUCopyPass* copyPass, const void* data, uint32 size, uint32 offset)
	{
		APOLLO_ASSERT(m_Handle, "Called UploadData on null buffer");

		GPUDevice& device = Renderer::GetInstance()->GetDevice();
		const SDL_GPUTransferBufferCreateInfo tBufferInfo{
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = size,
		};
		SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(
			device.GetHandle(),
			&tBufferInfo);
		APOLLO_ASSERT(transferBuffer, "Failed to create transfer buffer: {}", SDL_GetError());
		void* dest = SDL_MapGPUTransferBuffer(device.GetHandle(), transferBuffer, false);
		memcpy(dest, data, size);
		SDL_UnmapGPUTransferBuffer(device.GetHandle(), transferBuffer);

		const SDL_GPUTransferBufferLocation location{ .transfer_buffer = transferBuffer };
		const SDL_GPUBufferRegion destRegion{
			.buffer = m_Handle,
			.offset = offset,
			.size = size,
		};
		SDL_UploadToGPUBuffer(copyPass, &location, &destRegion, false);
		SDL_ReleaseGPUTransferBuffer(device.GetHandle(), transferBuffer);
	}
} // namespace apollo::rdr