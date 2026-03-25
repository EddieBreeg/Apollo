#pragma once

/** \file Scene.hpp */

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

	/**
	 * \brief Used to represent an object in a scene
	 */
	struct GameObject
	{
		ULID m_Id;
		std::string m_Name;
		entt::entity m_Entity;
		std::vector<const ecs::ComponentInfo*> m_Components;
	};

	/**
	 * \brief Scene asset type.
	 */
	class Scene : public IAsset
	{
	public:
		using IAsset::IAsset;
		GET_ASSET_TYPE_IMPL(EAssetType::Scene);

		/// Retrieves a specific game object from its ULID
		[[nodiscard]] const GameObject* GetGameObject(const ULID& id) const noexcept
		{
			if (const auto it = m_GameObjects.find(id); it != m_GameObjects.end())
				return &it->second;
			return nullptr;
		}

		/// Returns all game objets in the scene. Useful for UI inspectors
		[[nodiscard]] const ULIDMap<GameObject>& GetGameObjects() const noexcept
		{
			return m_GameObjects;
		}

		void Swap(Scene& other) noexcept { m_GameObjects.swap(other.m_GameObjects); }

		/**
		 * \brief Sends a dereferred reload request for this scene.
		 * \warning This should \b NOT be called on its own. Proper scene reloads should be handled
		 * through the \ref SceneLoadingSystem "scene loading system" using
		 * SceneSwitchRequestComponent
		 * \see [SceneSwitchRequestComponent](@ref SceneSwitchRequestComponent)
		 */
		APOLLO_API void ReloadDeferred(IAssetManager& assetManager);

	private:
		friend struct editor::AssetHelper<Scene>;
		ULIDMap<GameObject> m_GameObjects;
	};
} // namespace apollo
