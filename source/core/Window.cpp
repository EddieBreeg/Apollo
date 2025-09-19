#include "Window.hpp"

#include "Log.hpp"

#include <SDL3/SDL_video.h>

namespace brk {
	Window::Window(const WindowSettings& settings)
	{
		int32 flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
		if (settings.m_Resizeable)
			flags |= SDL_WINDOW_RESIZABLE;

		m_Handle = SDL_CreateWindow(
			settings.m_Title,
			(int32)settings.m_Width,
			(int32)settings.m_Height,
			flags);

		DEBUG_CHECK(m_Handle)
		{
			BRK_LOG_CRITICAL("Failed to create window: {}", SDL_GetError());
			return;
		}
	}

	Window::~Window()
	{
		if (m_Handle)
			SDL_DestroyWindow(m_Handle);
	}
} // namespace brk