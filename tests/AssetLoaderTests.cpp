#include <asset/Asset.hpp>
#include <asset/AssetLoader.hpp>
#include <asset/AssetManager.hpp>
#include <catch2/catch_test_macros.hpp>
#include <core/ThreadPool.hpp>
#include <rendering/Device.hpp>
#include <semaphore>

namespace apollo::asset_ut {
#define ASSET_LOADER_TEST(name) TEST_CASE(name, "[asset][asset_loader][mt]")

	const AssetMetadata g_DummyMeta{};

	struct TestAsset : public IAsset
	{
		TestAsset() { m_State = EAssetState::Loading; }

		GET_ASSET_TYPE_IMPL(EAssetType::Invalid);
	};

	EAssetLoadResult LoadDummy(IAsset&, const AssetMetadata&)
	{
		return EAssetLoadResult::Success;
	}

	struct Helper
	{
		Helper(bool registerCallback = true)
			: m_ThreadPool(1)
			, m_Loader(m_Dev, m_ThreadPool)
		{
			if (registerCallback)
			{
				m_Loader.RegisterCallback(
					[this]()
					{
						m_Sem.release();
					});
			}
		}

		TestAsset m_Dummy;
		rdr::GPUDevice m_Dev;
		mt::ThreadPool m_ThreadPool;
		AssetLoader m_Loader;
		std::binary_semaphore m_Sem{ 0 };

		AssetRef<IAsset> GetDummyRef() { return AssetRef<IAsset>{ &m_Dummy }; }
	};

	ASSET_LOADER_TEST("Load Asset")
	{
		Helper helper;
		AssetRef<IAsset> ref = helper.GetDummyRef();
		helper.m_Loader.AddRequest(
			AssetLoadRequest{
				ref,
				&LoadDummy,
				&g_DummyMeta,
			});
		helper.m_Loader.ProcessRequests();
		helper.m_Sem.acquire();
		CHECK(ref->IsLoaded());
	}

	ASSET_LOADER_TEST("Load Asset With Callback")
	{
		Helper helper;
		AssetRef<IAsset> ref = helper.GetDummyRef();
		EAssetState state;
		helper.m_Loader.AddRequest(
			AssetLoadRequest{
				ref,
				&LoadDummy,
				&g_DummyMeta,
				UniqueFunction<void(IAsset&)>{
					[&](const IAsset& asset)
					{
						state = asset.GetState();
					},
				},
			});
		helper.m_Loader.ProcessRequests();
		helper.m_Sem.acquire();
		CHECK(ref->IsLoaded());
		CHECK(state == EAssetState::Loaded);
	}

#undef ASSET_LOADER_TEST
} // namespace apollo::asset_ut