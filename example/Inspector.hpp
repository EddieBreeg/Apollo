#pragma once

#include <asset/AssetRef.hpp>
#include <asset/Scene.hpp>

namespace apollo {
	class Scene;

	struct GameObject;
} // namespace apollo

namespace apollo::editor {
	class AssetManager;
}

namespace apollo::demo {
	struct Inspector
	{
		editor::AssetManager* m_AssetManager = nullptr;
		AssetRef<Scene> m_Scene;
		const GameObject* m_CurrentObject = nullptr;
		bool m_ShowInspector = true;

		float3 m_Euler = {};

		void Update(entt::registry& registry);

	private:
		void InitTransform(const TransformComponent& comp);
	};
} // namespace apollo::demo