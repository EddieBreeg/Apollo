#pragma once

#include <PCH.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <numbers>

namespace apollo::demo {
	struct Camera
	{
		[[nodiscard]] float3 GetForward() const noexcept { return m_Fwd; }
		[[nodiscard]] float3 GetRight() const noexcept { return m_Right; }
		[[nodiscard]] float3 GetTranslate() const noexcept { return m_Translate; }

		glm::mat4x4 Update(float3 movement, float2 mouseMotion)
		{
			constexpr float maxPitch = .49f * std::numbers::pi_v<float>;

			const float yaw = m_Yaw - mouseMotion.x;
			const float pitch = Clamp(m_Pitch - mouseMotion.y, -maxPitch, maxPitch);

			if (yaw != m_Yaw)
			{
				ComputeForward();
				ComputeRight();
			}
			if (pitch != m_Pitch)
			{
				ComputeForward();
			}

			m_Pitch = pitch;
			m_Yaw = yaw;
			const float3 up = glm::cross(m_Right, m_Fwd);

			const float3 offset = movement.x * m_Right + movement.y * up + movement.z * m_Fwd;
			m_Translate += offset;
			// clang-format off
			return glm::mat4x4{
				m_Right.x, up.x, -m_Fwd.x, 0,
				m_Right.y, up.y, -m_Fwd.y, 0,
				m_Right.z, up.z, -m_Fwd.z, 0,
				-glm::dot(m_Right, m_Translate), 
				-glm::dot(up, m_Translate), 
				glm::dot(m_Fwd, m_Translate), 
				1,
			};
			// clang-format on
		}

		[[nodiscard]] const glm::mat4x4 GetLookAt() const noexcept
		{
			const float3 up = glm::cross(m_Right, m_Fwd);
			// clang-format off
			return glm::mat4x4{
				m_Right.x, up.x, -m_Fwd.x, 0,
				m_Right.y, up.y, -m_Fwd.y, 0,
				m_Right.z, up.z, -m_Fwd.z, 0,
				-glm::dot(m_Right, m_Translate), 
				-glm::dot(up, m_Translate), 
				glm::dot(m_Fwd, m_Translate), 
				1,
			};
			// clang-format on
		}

	private:
		void ComputeRight()
		{
			const float cr = std::cosf(m_Roll);
			const float sr = std::sinf(m_Roll);
			const float cy = std::cosf(m_Yaw);
			const float sy = std::sinf(m_Yaw);

			m_Right = float3{ cr * cy, sr, cr * -sy };
		}
		void ComputeForward()
		{
			const float cx = std::cosf(m_Pitch);
			const float cy = std::cosf(m_Yaw);
			const float sx = std::sinf(m_Pitch);
			const float sy = std::sinf(m_Yaw);

			m_Fwd = float3{ cx * -sy, sx, cx * -cy };
		}

		float3 m_Translate = {};
		float3 m_Right{ 1, 0, 0 }, m_Fwd{ 0, 0, -1 };
		float m_Roll = 0.0f;
		float m_Pitch = 0.0f;
		float m_Yaw = 0.0f;
	};
} // namespace apollo::demo