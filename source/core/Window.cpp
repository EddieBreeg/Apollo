#include "Window.hpp"

#include "Log.hpp"
#include "NumConv.hpp"

#include <SDL3/SDL_video.h>

namespace apollo {
	Window::Window(const WindowSettings& settings)
	{
		int32 flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
		if (settings.m_Resizeable)
			flags |= SDL_WINDOW_RESIZABLE;
		if (settings.m_Hidden)
			flags |= SDL_WINDOW_HIDDEN;

		m_Handle = SDL_CreateWindow(
			settings.m_Title,
			NumCast<int32>(settings.m_Width),
			NumCast<int32>(settings.m_Height),
			flags);

		DEBUG_CHECK(m_Handle)
		{
			APOLLO_LOG_CRITICAL("Failed to create window: {}", SDL_GetError());
			return;
		}
	}

	glm::uvec2 Window::GetSize() const
	{
		glm::uvec2 size;
		DEBUG_CHECK(m_Handle)
		{
			APOLLO_LOG_ERROR("Called GetSize on null window");
			return size;
		}
		SDL_GetWindowSize(
			m_Handle,
			reinterpret_cast<int32*>(&size.x),
			reinterpret_cast<int32*>(&size.y));
		return size;
	}

	Window::~Window()
	{
		if (m_Handle)
			SDL_DestroyWindow(m_Handle);
	}
} // namespace apollo