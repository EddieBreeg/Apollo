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
	EAssetLoadResult LoadScene(IAsset& out_asset, const AssetMetadata& metadata);
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

	private:
		friend EAssetLoadResult editor::LoadScene(IAsset& out_asset, const AssetMetadata& metadata);
		ULIDMap<GameObject> m_GameObjects;
	};
} // namespace apollo
