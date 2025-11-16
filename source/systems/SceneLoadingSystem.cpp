#include "SceneLoadingSystem.hpp"
#include <asset/AssetManager.hpp>
#include <core/Assert.hpp>
#include <core/Log.hpp>
#include <entt/entity/registry.hpp>

namespace {
	entt::registry g_TempWorld;
}

namespace apollo {
	SceneLoadingSystem::SceneLoadingSystem(IAssetManager& manager)
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
			ProcessLoadFinished(world);
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
		StartLoading(world, requestId);
	}

	void SceneLoadingSystem::StartLoading(entt::registry& world, const ULID& requestId)
	{
		const SceneComponent* currentSceneComp = nullptr;
		AssetRef<Scene> scene;
		if (m_SceneEntity != Unassigned<entt::entity>)
		{
			currentSceneComp = world.try_get<SceneComponent>(m_SceneEntity);
			DEBUG_CHECK(currentSceneComp && currentSceneComp->m_Scene && currentSceneComp->m_IsRoot)
			{
				APOLLO_LOG_ERROR(
					"Current scene entity either does not have a scene attached, or scene is not "
					"marked as root");
				return;
			}
			DEBUG_CHECK(currentSceneComp->m_Scene->IsLoaded())
			{
				APOLLO_LOG_ERROR(
					"Trying to switch away from scene {}, but scene is not loaded",
					currentSceneComp->m_Scene->GetId());
				return;
			}
		}

		if (currentSceneComp && currentSceneComp->m_Scene->GetId() == requestId)
		{
			scene = currentSceneComp->m_Scene;
			scene->ReloadDeferred(m_AssetManager);
		}
		else
		{
			scene = m_AssetManager.GetAsset<Scene>(requestId);
			if (!scene) [[unlikely]]
			{
				APOLLO_LOG_ERROR("Cannot switch to scene {}: no such scene", requestId);
				return;
			}
		}

		APOLLO_LOG_INFO("Switching to scene {}", requestId);

		m_SceneEntity = world.create();
		auto& sceneComp = world.emplace<SceneLoadComponent>(m_SceneEntity, std::move(scene));

		if (sceneComp.m_Scene->IsLoaded())
		{
			ProcessLoadFinished(world);
		}
		else
		{
			m_State = EState::Loading;
		}
	}

	void SceneLoadingSystem::ProcessLoadFinished(entt::registry& world)
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

	entt::registry& SceneLoadingSystem::GetTempWorld() noexcept
	{
		return g_TempWorld;
	}
} // namespace apollo