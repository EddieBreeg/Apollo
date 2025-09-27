#pragma once

#include <PCH.hpp>

struct SDL_Window;
union SDL_Event;

namespace brk {
	struct WindowSettings
	{
		const char* m_Title = "";
		uint32 m_Width = 1280;
		uint32 m_Height = 720;
		bool m_Resizeable = false;
	};

	class BRK_API Window
	{
	public:
		Window() = default;
		explicit Window(const WindowSettings& settings);
		~Window();

		Window(const Window&) = delete;
		Window(Window&& other) noexcept
			: m_Handle(other.m_Handle)
		{
			other.m_Handle = nullptr;
		}

		Window& operator=(const Window& other) = delete;
		Window& operator=(Window&& other) noexcept
		{
			Swap(other);
			return *this;
		}

		void Swap(Window& other) noexcept { std::swap(m_Handle, other.m_Handle); }

		[[nodiscard]] SDL_Window* GetHandle() const noexcept { return m_Handle; }
		[[nodiscard]] operator bool() const noexcept { return m_Handle; }

		[[nodiscard]] glm::uvec2 GetSize() const;

	private:
		SDL_Window* m_Handle = nullptr;
	};
} // namespace brk