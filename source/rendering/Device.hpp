#pragma once

/** \file Device.hpp */

#include <PCH.hpp>

#include "HandleWrapper.hpp"

struct SDL_GPUDevice;

namespace apollo {
	class Window;
}

namespace apollo::rdr {
	/**
	 * \brief Specifies the GPU backend to use
	 * \note Currently, only Vulkan is supported
	 * \todo Add support for D3D12 (this is quite low priority at the moment)
	 */
	enum class EBackend : int8
	{
		Invalid = -1,
		Vulkan = 0,
		D3D12,
		NBackends,
#ifdef APOLLO_VULKAN
		Default = Vulkan
#elif defined(APOLLO_D3D12)
		Default = D3D12
#endif
	};

	/// GPU device abstraction
	class APOLLO_API GPUDevice : public _internal::HandleWrapper<SDL_GPUDevice*>
	{
	public:
		using BaseType::BaseType;

		GPUDevice(EBackend backend, bool debugMode = false);
		GPUDevice(const GPUDevice&) = delete;

		GPUDevice& operator=(GPUDevice&& other)
		{
			Swap(static_cast<BaseType&>(other));
			return *this;
		}

		void Reset();

		bool ClaimWindow(Window& win);

		~GPUDevice();
	};
} // namespace apollo::rdr