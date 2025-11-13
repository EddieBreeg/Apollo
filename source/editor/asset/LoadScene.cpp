#include "AssetLoaders.hpp"
#include <asset/AssetManager.hpp>
#include <asset/Scene.hpp>
#include <core/Errno.hpp>
#include <core/Json.hpp>
#include <core/Log.hpp>
#include <ecs/ComponentRegistry.hpp>
#include <fstream>
#include <systems/SceneLoadingSystem.hpp>

namespace {
	bool LoadGameObject(
		apollo::GameObject& out_go,
		entt::registry& world,
		const nlohmann::json& json,
		const apollo::ecs::ComponentRegistry& registry)
	{
		if (!apollo::json::Visit(out_go.m_Id, json, "id"))
		{
			APOLLO_LOG_ERROR("Failed to load game object: no valid ULID");
			return false;
		}
		apollo::json::Visit(out_go.m_Name, json, "name");
		nlohmann::json compJson;
		if (!apollo::json::Visit(compJson, json, "components"))
		{
			APOLLO_LOG_ERROR("Failed to load game object {}: no components", out_go.m_Id);
			return false;
		}
		if (!compJson.is_object())
		{
			APOLLO_LOG_ERROR(
				"Failed to load game object {}: 'components' is not an object",
				out_go.m_Id);
			return false;
		}
		out_go.m_Entity = world.create();
		out_go.m_Components.reserve(compJson.size());

		for (auto it = compJson.begin(); it != compJson.end(); ++it)
		{
			std::string_view compName = it.key();
			const apollo::ecs::ComponentInfo* info = registry.GetInfo(compName);
			DEBUG_CHECK(info)
			{
				APOLLO_LOG_ERROR("Unknown component type: {}", compName);
				continue;
			}
			if (!info->m_Deserialize(out_go.m_Entity, world, &it.value()))
			{
				APOLLO_LOG_ERROR(
					"Component {} failed to load for GameObject {}",
					compName,
					out_go.m_Id);
				continue;
			}
			out_go.m_Components.emplace_back(info);
		}
		return true;
	}
} // namespace

namespace apollo::editor {
	template <>
	EAssetLoadResult AssetHelper<Scene>::Load(IAsset& out_asset, const AssetMetadata& metadata)
	{
		std::ifstream file{ metadata.m_FilePath, std::ios::in };
		if (!file.is_open())
		{
			APOLLO_LOG_ERROR(
				"Failed to load scene from {}: {}",
				metadata.m_FilePath.string(),
				GetErrnoMessage(errno));
			return EAssetLoadResult::Failure;
		}
		const nlohmann::json j = nlohmann::json::parse(file, nullptr, false);
		if (j.is_discarded())
		{
			APOLLO_LOG_ERROR("Failed to parse {} as JSON", metadata.m_FilePath.string());
			return EAssetLoadResult::Failure;
		}

		nlohmann::json objectsJson;
		// if no game objects: valid, we just have an empty scene
		if (!json::Visit(objectsJson, j, "gameObjects"))
			return EAssetLoadResult::Success;

		if (!objectsJson.is_array())
		{
			APOLLO_LOG_ERROR("Failed to load game objects from JSON: not an array");
			return EAssetLoadResult::Failure;
		}

		Scene& scene = static_cast<Scene&>(out_asset);

		entt::registry& world = SceneLoadingSystem::GetTempWorld();
		const auto& registry = *ecs::ComponentRegistry::GetInstance();

		GameObject object;
		for (const nlohmann::json& o : objectsJson)
		{
			if (LoadGameObject(object, world, o, registry))
				scene.m_GameObjects.emplace(object.m_Id, std::move(object));
		}
		return EAssetLoadResult::Success;
	}
} // namespace apollo::editor