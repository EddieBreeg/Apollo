#include "Texture.hpp"
#include "Renderer.hpp"
#include <SDL3/SDL_gpu.h>

namespace brk::rdr {
	Texture2D::~Texture2D()
	{
		if (m_Handle)
			SDL_ReleaseGPUTexture(Renderer::GetInstance()->GetDevice().GetHandle(), m_Handle);
	}
} // namespace brk::rdr