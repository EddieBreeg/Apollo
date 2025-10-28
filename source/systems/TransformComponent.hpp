#pragma once

#include <PCH.hpp>
#include <core/Transform.hpp>
#include <ecs/Reflection.hpp>

namespace apollo {
	struct TransformComponent
	{
		float3 m_Position = {};
		float3 m_Scale = { 1, 1, 1 };
		glm::quat m_Rotation;

		static constexpr ecs::ComponentReflection<
			&TransformComponent::m_Position,
			&TransformComponent::m_Scale,
			&TransformComponent::m_Rotation>
			Reflection{ "transform", { "position", "scale", "rotation" } };
	};
} // namespace apollo