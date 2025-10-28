#include <SDL3/SDL_init.h>
#include <asset/AssetManager.hpp>
#include <asset/AssetRef.hpp>
#include <asset/Scene.hpp>
#include <catch2/catch_test_macros.hpp>
#include <core/Assert.hpp>
#include <core/GameTime.hpp>
#include <core/ThreadPool.hpp>
#include <core/ULIDFormatter.hpp>
#include <ecs/Manager.hpp>
#include <rendering/Device.hpp>
#include <semaphore>
#include <systems/SceneLoadingSystem.hpp>

namespace apollo::scene_ut {
	[[maybe_unused]] constexpr ULID g_AssetId1 = "01K8GV45XSRQ7DRN15KFESCFXY"_ulid;
	[[maybe_unused]] constexpr ULID g_AssetId2 = "01K8K8AWNQZWN3XGJ07GTFN0ED"_ulid;

	bool CreateMetadata(const std::filesystem::path&, ULIDMap<AssetMetadata>& out_bank)
	{
		out_bank.emplace(
			std::pair{
				g_AssetId1,
				AssetMetadata{ .m_Id = g_AssetId1, .m_Type = EAssetType::Scene },
			});
		out_bank.emplace(
			std::pair{
				g_AssetId2,
				AssetMetadata{ .m_Id = g_AssetId2, .m_Type = EAssetType::Scene },
			});
		return true;
	}

	EAssetLoadResult LoadScene(IAsset&, const AssetMetadata&)
	{
		return EAssetLoadResult::Success;
	}

	EAssetLoadResult LoadSceneWithSubscene(IAsset&, const AssetMetadata& metadata)
	{
		if (metadata.m_Id != g_AssetId1)
			return EAssetLoadResult::Success;

		// We artificially load a subscene within the one we're already loading
		auto& tempWorld = SceneLoadingSystem::GetTempWorld();
		auto subScene = AssetManager::GetInstance()->GetAsset<Scene>(g_AssetId2);
		APOLLO_ASSERT(subScene, "Scene {} not found", g_AssetId2);
		tempWorld.emplace<SceneComponent>(tempWorld.create(), std::move(subScene));
		return EAssetLoadResult::Success;
	}

	struct TestHelper
	{
		TestHelper(AssetImportFunc* loadScene = &LoadScene)
			: m_ThreadPool(1)
			, m_EcsManager(ecs::Manager::Init())
			, m_Semaphore(0)
		{
			APOLLO_LOG_INFO("Test Start");
			SDL_Init(SDL_INIT_VIDEO);
			m_Device = rdr::GPUDevice{ rdr::EBackend::Default };
			m_AssetManager = &AssetManager::Init(
				AssetManagerSettings{
					.m_MetadataImportFunc = CreateMetadata,
					.m_LoadScene = loadScene,
					.m_AssetPath = "./assets",
				},
				m_Device,
				m_ThreadPool);
			m_AssetManager->ImportMetadataBank();
			m_AssetManager->GetAssetLoader().RegisterCallback(
				[this]()
				{
					m_Semaphore.release();
				});
			m_EcsManager.AddSystem<SceneLoadingSystem>(*m_AssetManager);
			m_EcsManager.PostInit();

			m_GameTime.Reset();
		}

		entt::registry& GetWorld() noexcept { return m_EcsManager.GetEntityWorld(); }

		~TestHelper()
		{
			APOLLO_LOG_INFO("Test End");
			ecs::Manager::Shutdown();
			AssetManager::Shutdown();
			m_Device.Reset();
			SDL_Quit();
		}

		void Update()
		{
			m_AssetManager->Update();
			m_EcsManager.Update(m_GameTime);
			m_GameTime.Update();
		}

		rdr::GPUDevice m_Device;
		mt::ThreadPool m_ThreadPool;
		ecs::Manager& m_EcsManager;
		AssetManager* m_AssetManager = nullptr;
		std::binary_semaphore m_Semaphore;
		GameTime m_GameTime;
	};

#define SCENE_TEST(name) TEST_CASE(name, "[scene]")

	SCENE_TEST("Load Scene (without loading system)")
	{
		TestHelper helper;
		AssetRef<Scene> scene = helper.m_AssetManager->GetAsset<Scene>(g_AssetId1);
		REQUIRE(scene);
		CHECK(scene->GetState() == EAssetState::Loading);

		helper.Update();
		helper.m_Semaphore.acquire();
		CHECK(scene->GetState() == EAssetState::Loaded);
	}

	SCENE_TEST("No switch request")
	{
		TestHelper helper;
		helper.Update();
		CHECK(helper.GetWorld().view<const SceneComponent>().empty());
		CHECK(helper.GetWorld().view<const SceneLoadComponent>().empty());
	}

	SCENE_TEST("Request Scene Load")
	{
		TestHelper helper;
		auto& world = helper.GetWorld();
		world.emplace<SceneSwitchRequestComponent>(world.create(), g_AssetId1);
		helper.Update();
		{
			const auto view = world.view<const SceneLoadComponent>();
			REQUIRE(!view.empty());
			const SceneLoadComponent& loadComp = *view->begin();
			CHECK(loadComp.m_Scene);
			CHECK(loadComp.m_Scene->GetId() == g_AssetId1);
			CHECK(loadComp.m_Scene->GetState() == EAssetState::Loading);
		}
		{
			const auto view = world.view<const SceneSwitchRequestComponent>();
			CHECK(view.empty());
		}
	}

	SCENE_TEST("Ignore request when scene already loading")
	{
		TestHelper helper;
		auto& world = helper.GetWorld();
		world.emplace<SceneSwitchRequestComponent>(world.create(), g_AssetId1);
		helper.Update();
		// create a second request while the first one is pending
		world.emplace<SceneSwitchRequestComponent>(world.create(), g_AssetId2);
		// update the ECS system, not the asset manager
		helper.m_EcsManager.Update(helper.m_GameTime);
		{
			const auto view = world.view<const SceneLoadComponent>();
			CHECK(view->size() == 1);
		}
		{
			const auto view = world.view<const SceneSwitchRequestComponent>();
			REQUIRE(!view.empty());
			CHECK(view->begin()->m_Id == g_AssetId2);
		}
	}

	SCENE_TEST("SceneLoadFinished Event")
	{
		TestHelper helper;
		auto& world = helper.GetWorld();
		world.emplace<SceneSwitchRequestComponent>(world.create(), g_AssetId1);
		helper.Update();
		helper.m_AssetManager->Update();
		helper.m_Semaphore.acquire(); // wait for the loading to finish
		helper.Update();

		entt::entity sceneEntity;
		AssetRef<Scene> scene;

		{
			const auto view = world.view<const SceneLoadFinishedEventComponent>();
			REQUIRE(!view->empty());
			sceneEntity = *view->begin();
			const SceneComponent* sceneComp = world.try_get<const SceneComponent>(sceneEntity);
			REQUIRE(sceneComp);
			CHECK(sceneComp->m_IsRoot);

			scene = sceneComp->m_Scene;
			CHECK(scene);
			CHECK(scene->GetId() == g_AssetId1);
			CHECK(scene->GetState() == EAssetState::Loaded);
		}
		helper.Update();

		CHECK(world.view<const SceneLoadFinishedEventComponent>().empty());
		const SceneComponent* sceneComp = world.try_get<const SceneComponent>(sceneEntity);
		REQUIRE(sceneComp);
		CHECK(sceneComp->m_IsRoot);
		CHECK(sceneComp->m_Scene == scene);
	}

	SCENE_TEST("Load scene with subscene")
	{
		TestHelper helper{ LoadSceneWithSubscene };
		auto& world = helper.GetWorld();
		world.emplace<SceneSwitchRequestComponent>(world.create(), g_AssetId1);
		helper.Update();
		helper.m_AssetManager->Update();
		helper.m_Semaphore.acquire(); // wait for the loading to finish
		helper.Update();

		const SceneComponent* subSceneComp = nullptr;
		uint32 subSceneCount = 0;
		{
			const auto view = world.view<const SceneComponent>(
				entt::exclude_t<SceneLoadFinishedEventComponent>{});
			for (const auto subScene : view)
			{
				++subSceneCount;
				subSceneComp = &view.get<const SceneComponent>(subScene);
			}
		}
		REQUIRE(subSceneCount == 1);
		REQUIRE(subSceneComp);
		CHECK(!subSceneComp->m_IsRoot);
		CHECK(subSceneComp->m_Scene);
		CHECK(subSceneComp->m_Scene->GetId() == g_AssetId2);
		CHECK(subSceneComp->m_Scene->GetState() == EAssetState::Loaded);
	}

	SCENE_TEST("Switch from scene1 to scene2")
	{
		TestHelper helper{};
		auto& world = helper.GetWorld();
		AssetRef<Scene> scene = helper.m_AssetManager->GetAsset<Scene>(g_AssetId1);
		REQUIRE(scene);
		helper.m_AssetManager->Update();
		helper.m_Semaphore.acquire();
		world.emplace<SceneComponent>(world.create(), std::move(scene), true);
		world.emplace<SceneSwitchRequestComponent>(world.create(), g_AssetId2);

		helper.Update();
		helper.m_AssetManager->Update();
		helper.m_Semaphore.acquire(); // wait for the second scene to finish loading
		helper.Update();
		helper.m_AssetManager->Update();

		{
			const auto view = world.view<const SceneComponent>();
			REQUIRE(view.size() == 1);
			const auto& sceneComp = *view->begin();
			CHECK(sceneComp.m_IsRoot);
			CHECK(sceneComp.m_Scene);
			CHECK(sceneComp.m_Scene->GetId() == g_AssetId2);
			CHECK(sceneComp.m_Scene->GetState() == EAssetState::Loaded);
		}
	}

#undef SCENE_TEST
} // namespace apollo::scene_ut