#pragma once

#include <PCH.hpp>

#include "HandleWrapper.hpp"

struct SDL_GPUDevice;

namespace brk
{
	class Window;
}

namespace brk::rdr {
	enum class EBackend : int8
	{
		Invalid = -1,
		Vulkan = 0,
		D3D12,
		NBackends,
#ifdef BRK_VULKAN
		Default = Vulkan
#elif defined(BRK_D3D12)
		Default = D3D12
#endif
	};

	class BRK_API GPUDevice : public _internal::HandleWrapper<SDL_GPUDevice*>
	{
	public:
		using BaseType::BaseType;

		GPUDevice(EBackend backend, bool debugMode = false);
		GPUDevice(const GPUDevice&) = delete;

		GPUDevice& operator=(GPUDevice& other)
		{
			Swap(static_cast<BaseType&>(other));
			return *this;
		}

		bool ClaimWindow(Window& win);

		~GPUDevice();
	};
} // namespace brk::rdr