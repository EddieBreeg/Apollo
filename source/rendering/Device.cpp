#include "Device.hpp"

#include <SDL3/SDL_gpu.h>
#include <core/Assert.hpp>
#include <core/Enum.hpp>
#include <core/Window.hpp>

#ifdef APOLLO_VULKAN
#include <vulkan/vulkan_core.h>

namespace {
	VkPhysicalDeviceVulkan11Features g_Vk11Features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
		.shaderDrawParameters = true,
	};

	SDL_GPUVulkanOptions g_VkOptions{
		.vulkan_api_version = VK_API_VERSION_1_2,
		.feature_list = &g_Vk11Features,
	};
} // namespace
#endif

namespace apollo::rdr {

	GPUDevice::GPUDevice(EBackend backend, bool debugMode)
	{
		SDL_PropertiesID deviceProps = SDL_CreateProperties();
		SDL_SetBooleanProperty(deviceProps, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, debugMode);

		switch (backend)
		{
#ifdef APOLLO_VULKAN
		case EBackend::Vulkan:
			SDL_SetBooleanProperty(
				deviceProps,
				SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN,
				true);
			SDL_SetPointerProperty(
				deviceProps,
				SDL_PROP_GPU_DEVICE_CREATE_VULKAN_OPTIONS_POINTER,
				&g_VkOptions);
			break;
#endif
#ifdef APOLLO_D3D12
		case EBackend::D3D12:
			SDL_SetBooleanProperty(
				deviceProps,
				SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN,
				true);
			m_Handle = SDL_CreateGPUDeviceWithProperties(deviceProps);
			break;
#endif
		default:
			APOLLO_LOG_CRITICAL("Unsupported rendering backend {}", ToUnderlying(backend));
			DEBUG_BREAK();
		}
		m_Handle = SDL_CreateGPUDeviceWithProperties(deviceProps);
		SDL_DestroyProperties(deviceProps);

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