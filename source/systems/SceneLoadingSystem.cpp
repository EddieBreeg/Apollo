#include "SceneLoadingSystem.hpp"
#include <asset/AssetManager.hpp>
#include <core/Log.hpp>
#include <entt/entity/registry.hpp>

namespace {
	entt::registry g_TempWorld;
}

namespace apollo {
	SceneLoadingSystem::SceneLoadingSystem(AssetManager& manager)
		: m_AssetManager(manager)
	{}
	void SceneLoadingSystem::PostInit()
	{
		m_AssetManager.GetAssetLoader().RegisterCallback(
			[this]()
			{
				EState temp = EState::Loading;
				m_State.compare_exchange_strong(temp, EState::Finished);
			});
	}

	void SceneLoadingSystem::Update(entt::registry& world, const GameTime&)
	{
		for (const auto evt : world.view<const SceneLoadFinishedEventComponent>())
			world.remove<SceneLoadFinishedEventComponent>(evt);

		if (m_State == EState::Finished)
		{
			m_State = EState::Default;

			const auto* loadComp = world.try_get<const SceneLoadComponent>(m_SceneEntity);
			DEBUG_CHECK(loadComp)
			{
				APOLLO_LOG_ERROR(
					"Scene Load entity has been destroyed before the scene was fully loaded!");
				return;
			}
			// Something went wrong when loading the scene: abort
			if (!loadComp->m_Scene->IsLoaded())
				return;

			APOLLO_LOG_INFO("Scene {} loaded successfully", loadComp->m_Scene->GetId());

			m_SceneEntity = g_TempWorld.create();
			g_TempWorld.emplace<SceneComponent>(m_SceneEntity, std::move(loadComp->m_Scene), true);
			world.swap(g_TempWorld);
			g_TempWorld.clear();
			world.emplace<SceneLoadFinishedEventComponent>(m_SceneEntity);
		}

		if (m_State == EState::Loading)
			return;

		ULID requestId;
		{
			const auto reqView = world.view<const SceneSwitchRequestComponent>();
			if (reqView.empty())
				return;

			requestId = reqView->begin()->m_Id;
			for (const auto req : reqView)
				world.destroy(req);
		}
		DEBUG_CHECK(requestId)
		{
			APOLLO_LOG_ERROR("Received scene switch request with invalid scene id");
			return;
		}

		AssetRef<Scene> scene = m_AssetManager.GetAsset<Scene>(requestId);
		if (!scene) [[unlikely]]
		{
			APOLLO_LOG_ERROR("Cannot switch to scene {}: no such scene", requestId);
			return;
		}
		APOLLO_LOG_INFO("Switching to scene {}", requestId);

		m_State = EState::Loading;
		m_SceneEntity = world.create();
		world.emplace<SceneLoadComponent>(m_SceneEntity, std::move(scene));
	}

	entt::registry& SceneLoadingSystem::GetTempWorld() noexcept
	{
		return g_TempWorld;
	}
} // namespace apollo