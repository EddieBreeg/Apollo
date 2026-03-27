//! [transform]
struct TransformComponent
{
	float3 m_Position = {};
	float3 m_Scale = { 1, 1, 1 };
	glm::quat m_Rotation;

	static constexpr apollo::ecs::ComponentReflection<
		&TransformComponent::m_Position,
		&TransformComponent::m_Scale,
		&TransformComponent::m_Rotation>
		Reflection{
			// the component's name:
			"transform",
			// the fields' names:
			{
				"position",
				"scale",
				"rotation",
			},
		};
};
//! [transform]

//! [registration]
void Register(apollo::ecs::ComponentRegistry& registry)
{
	registry.RegisterComponent<TransformComponent>(/* no argument required here */);
}
//! [registration]
