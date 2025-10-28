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

namespace apollo::rdr {
	GPUDevice::GPUDevice(EBackend backend, bool debugMode)
	{
		APOLLO_ASSERT(
			(backend > EBackend::Invalid && backend < EBackend::NBackends),
			"Invalid backend: {}",
			ToUnderlying(backend));
		m_Handle = SDL_CreateGPUDevice(
			g_SDLShaderFormats[ToUnderlying(backend)],
			debugMode,
			nullptr);
		DEBUG_CHECK(m_Handle)
		{
			APOLLO_LOG_CRITICAL("Failed to create GPU device: {}", SDL_GetError());
			return;
		}
	}

	void GPUDevice::Reset()
	{
		if (m_Handle)
		{
			APOLLO_LOG_TRACE("Cleaning up GPU device");
			SDL_DestroyGPUDevice(m_Handle);
			m_Handle = nullptr;
		}
	}

	GPUDevice::~GPUDevice()
	{
		Reset();
	}

	bool GPUDevice::ClaimWindow(Window& win)
	{
		return SDL_ClaimWindowForGPUDevice(m_Handle, win.GetHandle());
	}
} // namespace apollo::rdr