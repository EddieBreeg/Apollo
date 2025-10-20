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
} // namespace apollo::rdr