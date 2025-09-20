#include "Device.hpp"

#include <SDL3/SDL_gpu.h>
#include <core/Assert.hpp>
#include <core/Enum.hpp>
#include <core/Window.hpp>

namespace {
	static constexpr uint32 g_SDLShaderFormats[] = {
		SDL_GPU_SHADERFORMAT_SPIRV,
		SDL_GPU_SHADERFORMAT_DXIL,
	};
}

namespace brk::rdr {
	GPUDevice::GPUDevice(EBackend backend, bool debugMode)
	{
		BRK_ASSERT(
			(backend > EBackend::Invalid && backend < EBackend::NBackends),
			"Invalid backend: {}",
			ToUnderlying(backend));
		m_Handle = SDL_CreateGPUDevice(
			g_SDLShaderFormats[ToUnderlying(backend)],
			debugMode,
			nullptr);
		DEBUG_CHECK(m_Handle)
		{
			BRK_LOG_CRITICAL("Failed to create GPU device: {}", SDL_GetError());
			return;
		}
	}

	GPUDevice::~GPUDevice()
	{
		BRK_LOG_TRACE("Cleaning up GPU device");
		if (m_Handle)
			SDL_DestroyGPUDevice(m_Handle);
	}

	bool GPUDevice::ClaimWindow(Window& win)
	{
		return SDL_ClaimWindowForGPUDevice(m_Handle, win.GetHandle());
	}
} // namespace brk::rdr