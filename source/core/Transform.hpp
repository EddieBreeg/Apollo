#pragma once

#include <PCH.hpp>
#include <glm/gtc/quaternion.hpp>

namespace apollo {

	[[nodiscard]] inline glm::mat4x4 ComputeTransformMatrix(
		const float3& pos,
		const float3& scale,
		const glm::quat& rot) noexcept
	{
		glm::mat4x4 m = glm::mat4_cast(rot);
		m[0][0] *= scale.x;
		m[1][1] *= scale.y;
		m[2][2] *= scale.z;
		m[3] = float4{ pos, 1 };
		return m;
	}
} // namespace apollo