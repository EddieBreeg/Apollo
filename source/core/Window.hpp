#pragma once

#include <PCH.hpp>

struct SDL_Window;
union SDL_Event;

/** \file Window.hpp */

namespace apollo {
	/// Settings used to initialize the main app window
	struct WindowSettings
	{
		const char* m_Title = "";
		uint32 m_Width = 1280;
		uint32 m_Height = 720;
		bool m_Resizeable = false;
		bool m_Hidden = false;
	};

	/// Main window class
	class APOLLO_API Window
	{
	public:
		/// Creates a "null" window. After calling this constructor, `(bool)*this` returns false
		Window() = default;
		/**
		 * \brief Creates a window with the provided settings.
		 * \note The main window is owned by the App instance. You should probably not be
		 * instantiating this class on your own unless you know what you're doing.
		 */
		explicit Window(const WindowSettings& settings);
		~Window();

		Window(const Window&) = delete;
		/**
		 * \brief Moves a window. After this call, `other` is left in a valid state as if default
		 * constructed.
		 */
		Window(Window&& other) noexcept
			: m_Handle(other.m_Handle)
		{
			other.m_Handle = nullptr;
		}

		Window& operator=(const Window& other) = delete;

		/// Calls `Swap(other)`
		Window& operator=(Window&& other) noexcept
		{
			Swap(other);
			return *this;
		}

		/**
		 * \brief Sets the mouse cursor's relative mode state.
		 * See the [SDL documentation for
		 * SDL_SetWindowRelativeMouseMode](https://wiki.libsdl.org/SDL3/SDL_SetWindowRelativeMouseMode)
		 * for details.
		 */
		bool SetCursorRelativeMode(bool enable = true) const;

		void Swap(Window& other) noexcept { std::swap(m_Handle, other.m_Handle); }

		[[nodiscard]] SDL_Window* GetHandle() const noexcept { return m_Handle; }
		/**
		 * \returns (bool)m_Handle
		 */
		[[nodiscard]] operator bool() const noexcept { return m_Handle; }

		[[nodiscard]] glm::uvec2 GetSize() const;

	private:
		SDL_Window* m_Handle = nullptr;
	};
} // namespace apollo