#pragma once

#include <PCH.hpp>

#include "Asset.hpp"
#include "asset/AssetFunctions.hpp"
#include "core/Map.hpp"
#include <core/ULID.hpp>
#include <entt/entity/fwd.hpp>
#include <string>
#include <vector>

namespace apollo::editor {
	template <class A>
	struct AssetHelper;
}

namespace apollo {
	namespace ecs {
		struct ComponentInfo;

	}

	struct GameObject
	{
		ULID m_Id;
		std::string m_Name;
		entt::entity m_Entity;
		std::vector<const ecs::ComponentInfo*> m_Components;
	};

	class Scene : public IAsset
	{
	public:
		using IAsset::IAsset;
		GET_ASSET_TYPE_IMPL(EAssetType::Scene);
		[[nodiscard]] const GameObject* GetGameObject(const ULID& id) const noexcept
		{
			if (const auto it = m_GameObjects.find(id); it != m_GameObjects.end())
				return &it->second;
			return nullptr;
		}

		[[nodiscard]] const ULIDMap<GameObject>& GetGameObjects() const noexcept
		{
			return m_GameObjects;
		}

		void Swap(Scene& other) noexcept
		{
			m_GameObjects.swap(other.m_GameObjects);
		}

		/**
		* \warning: This should NOT be called on its own. To properly reload a scene, use the scene loading system
		* \see [SceneSwitchRequestComponent](@ref SceneSwitchRequestComponent)
		*/
		APOLLO_API void ReloadDeferred(IAssetManager& assetManager);

	private:
		friend struct editor::AssetHelper<Scene>;
		ULIDMap<GameObject> m_GameObjects;
	};
} // namespace apollo
