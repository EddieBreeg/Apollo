#pragma once

#include <PCH.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

namespace apollo::demo {
	struct Camera
	{
		float3 m_Translate = {};
		float m_Roll = 0.0f;
		float m_Pitch = 0.0f;
		float m_Yaw = 0.0f;

		[[nodiscard]] float3 GetForward() const noexcept
		{
			const float cx = std::cosf(m_Pitch);
			const float cy = std::cosf(m_Yaw);
			const float sx = std::sinf(m_Pitch);
			const float sy = std::sinf(m_Yaw);

			return float3{ cx * -sy, sx, cx * -cy };
		}

		[[nodiscard]] float3 GetRight() const noexcept
		{
			const float cr = std::cosf(m_Roll);
			const float sr = std::sinf(m_Roll);
			const float cy = std::cosf(m_Yaw);
			const float sy = std::sinf(m_Yaw);

			return float3{ cr * cy, sr, cr * -sy };
		}

		[[nodiscard]] glm::mat4x4 GetLookAt() const noexcept
		{
			const float3 fwd = GetForward();
			const float3 right = glm::normalize(glm::cross(fwd, { 0, 1, 0 }));
			const float3 up = glm::cross(right, fwd);

			// clang-format off
			return glm::mat4x4{
				right.x, up.x, -fwd.x, 0,
				right.y, up.y, -fwd.y, 0,
				right.z, up.z, -fwd.z, 0,
				-glm::dot(right, m_Translate), 
				-glm::dot(up, m_Translate), 
				glm::dot(fwd, m_Translate), 
				1,
			};
			// clang-format on
		}
	};
} // namespace apollo::demo