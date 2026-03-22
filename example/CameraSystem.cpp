#include "CameraSystem.hpp"

namespace apollo::demo {
	void CameraSystem::Update(entt::registry& world, const GameTime& gameTime)
	{
		using namespace apollo::inputs;

		float2 mouseMotion = {};
		if (!m_CameraLocked)
		{
			const auto view = world.view<const inputs::MouseMotionEventComponent>();
			for (const auto e : view)
			{
				mouseMotion += view.get<const inputs::MouseMotionEventComponent>(e).m_Motion;
			}
			mouseMotion *= gameTime.GetDelta().count() * m_MouseSpeed;
		}

		const auto view = world.view<const inputs::KeyDownEventComponent>();
		for (const auto e : view)
		{
			const auto& comp = view->get(e);
			switch (comp.m_Key)
			{
			case inputs::EKey::F1:
				if (comp.m_Repeat)
					break;

				DEBUG_CHECK(m_Window.SetCursorRelativeMode(m_CameraLocked))
				{
					APOLLO_LOG_ERROR("Failed to set relative cursor mode: {}", SDL_GetError());
				}
				m_CameraLocked = !m_CameraLocked;
				break;
			default: break;
			}
		}
		const bool* keyStates = inputs::GetKeyStates();
		const float3 movement{
			float(keyStates[SDL_SCANCODE_D] - keyStates[SDL_SCANCODE_A]),
			float(keyStates[SDL_SCANCODE_Q] - keyStates[SDL_SCANCODE_E]),
			float(keyStates[SDL_SCANCODE_W] - keyStates[SDL_SCANCODE_S]),
		};

		const float moveSpeed = gameTime.GetDelta().count() * m_CameraSpeed *
								(1.0f + keyStates[SDL_SCANCODE_LSHIFT]);
		m_ViewMatrix = m_Camera.Update(moveSpeed * movement, mouseMotion);
	}
} // namespace apollo::demo