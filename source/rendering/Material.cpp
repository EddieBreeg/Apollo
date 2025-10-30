#include "Material.hpp"
#include "Context.hpp"
#include <SDL3/SDL_gpu.h>

namespace apollo::rdr {
	Material::~Material()
	{
		if (m_Handle)
		{
			SDL_ReleaseGPUGraphicsPipeline(
				Context::GetInstance()->GetDevice().GetHandle(),
				static_cast<SDL_GPUGraphicsPipeline*>(m_Handle));
		}
	}

	void MaterialInstance::PushFragmentConstants(SDL_GPUCommandBuffer* cmdBuffer, uint32 index)
	{
		APOLLO_ASSERT(index < 4, "Block index {} is out of bounds", index);
		SDL_PushGPUFragmentUniformData(
			cmdBuffer,
			index,
			m_FragmentConstants[index].m_Buf,
			m_BlockSizes[index]);
	}
} // namespace apollo::rdr