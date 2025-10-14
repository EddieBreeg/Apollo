#include "Material.hpp"
#include "Renderer.hpp"
#include <SDL3/SDL_gpu.h>

namespace apollo::rdr {
	Material::~Material()
	{
		if (m_Handle)
		{
			SDL_ReleaseGPUGraphicsPipeline(
				Renderer::GetInstance()->GetDevice().GetHandle(),
				static_cast<SDL_GPUGraphicsPipeline*>(m_Handle));
		}
	}
} // namespace apollo::rdr