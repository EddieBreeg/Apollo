#pragma once

#include <PCH.hpp>
#include <core/Singleton.hpp>

#include "Device.hpp"

struct SDL_Window;

namespace brk {
	class Window;
}

namespace brk::rdr {
	class BRK_API Renderer : public Singleton<Renderer>
	{
	public:
		~Renderer() = default;

		GPUDevice& GetDevice() noexcept { return m_Device; }

	private:
		Renderer(EBackend backend, Window& window, bool gpuDebug = false);
		friend class Singleton<Renderer>;

		static std::unique_ptr<Renderer> s_Instance;

		GPUDevice m_Device;
		Window& m_Window;
	};
} // namespace brk::rdr