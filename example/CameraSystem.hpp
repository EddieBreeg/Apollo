#pragma once

#include "Camera.hpp"

namespace apollo::demo {
	class CameraSystem
	{
	public:
		CameraSystem(Window& window)
			: m_Window(window)
		{}

		[[nodiscard]] const glm::mat4x4& GetViewMatrix() const noexcept { return m_ViewMatrix; }
		[[nodiscard]] const Camera& GetCamera() const noexcept { return m_Camera; }

		void Update(entt::registry&, const GameTime&);

		float m_CameraSpeed = 1.0f;
		float m_MouseSpeed = 3.0f;

	private:
		Window& m_Window;
		Camera m_Camera;
		float3 m_Forward{ 0, 0, -1 };
		float3 m_Right{ 1, 0, 0 };
		glm::mat4x4 m_ViewMatrix = glm::identity<glm::mat4x4>();
		bool m_CameraLocked = true;
	};
} // namespace apollo::demo