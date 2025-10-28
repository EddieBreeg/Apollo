#include <PCH.hpp>
#include <asset/AssetRef.hpp>
#include <asset/Scene.hpp>

#include <ecs/Reflection.hpp>

namespace apollo {
	// Represents a scene that is currently loaded
	struct SceneComponent
	{
		AssetRef<Scene> m_Scene;
		bool m_IsRoot = false;

		static constexpr ecs::ComponentReflection<&SceneComponent::m_Scene> Reflection{
			"scene",
			{ "ref" },
		};
	};

	// Represents a scene that is being loaded
	struct SceneLoadComponent
	{
		AssetRef<Scene> m_Scene;
	};

	// Used to request a scene swap. This is the primary way scenes should be loaded
	struct SceneSwitchRequestComponent
	{
		ULID m_Id;
	};

	// Gets added to the root scene for one frame after loading completes
	struct SceneLoadFinishedEventComponent
	{};
} // namespace apollo