#include "Renderer.hpp"
#include <SDL3/SDL_gpu.h>
#include <core/Assert.hpp>
#include <core/Log.hpp>
#include <core/Window.hpp>

namespace brk::rdr {
	std::unique_ptr<Renderer> Renderer::s_Instance;

	Renderer::Renderer(EBackend backend, Window& window, bool gpuDebug)
		: m_Device(backend, gpuDebug)
		, m_Window(window)
	{
		if (!m_Device) [[unlikely]]
			return;

		// it is valid to initialized the renderer without a window, we can use offline rendering
		if (!window)
			return;

		DEBUG_CHECK(m_Device.ClaimWindow(window))
		{
			BRK_LOG_CRITICAL("Failed to claim window for GPU device: {}", SDL_GetError());
		}
	}
} // namespace brk::rdr