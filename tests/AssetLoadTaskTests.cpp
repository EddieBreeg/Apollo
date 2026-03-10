#include <asset/AssetLoader.hpp>
#include <catch2/catch_test_macros.hpp>

namespace apollo::asset_ut {
	struct TestAsset : public IAsset
	{
		GET_ASSET_TYPE_IMPL(EAssetType::Invalid);
		TestAsset(EAssetState state = EAssetState::Loading) { m_State = state; }

		void SetState(EAssetState state) noexcept { m_State.store(state); }
		void Swap(TestAsset&) {};
	};

	AssetLoadTask AwaitAndReturn(AssetRef<TestAsset> await) noexcept
	{
		co_await await;
		co_return await && await->IsLoaded();
	}

#define LOAD_TEST(name) TEST_CASE(name, "[asset_load_task]")

	LOAD_TEST("Load success")
	{
		AssetLoadTask task = []() -> AssetLoadTask
		{
			co_return true;
		}();

		REQUIRE(task);
		CHECK(task() == EAssetLoadResult::Success);
	}

	LOAD_TEST("Load failure")
	{
		auto task = []() -> AssetLoadTask
		{
			co_return false;
		}();
		REQUIRE(task);
		CHECK(task() == EAssetLoadResult::Failure);
	}

	LOAD_TEST("Await Loaded Asset")
	{
		TestAsset asset{ EAssetState::Loaded };
		AssetLoadTask task = AwaitAndReturn(AssetRef<TestAsset>{ &asset });
		CHECK(task() == EAssetLoadResult::Success);
	}
	LOAD_TEST("Await Null Asset")
	{
		AssetLoadTask task = AwaitAndReturn(AssetRef<TestAsset>{});
		CHECK(task() == EAssetLoadResult::Failure);
	}
	LOAD_TEST("Await Invalid Asset")
	{
		TestAsset asset{ EAssetState::Invalid };
		AssetLoadTask task = AwaitAndReturn(AssetRef<TestAsset>{ &asset });
		CHECK(task() == EAssetLoadResult::Failure);
	}
	LOAD_TEST("Deferred Load Success")
	{
		TestAsset asset;
		AssetLoadTask task = AwaitAndReturn(AssetRef<TestAsset>{ &asset });
		CHECK(task() == EAssetLoadResult::TryAgain);
		CHECK(task() == EAssetLoadResult::TryAgain);
		asset.SetState(EAssetState::Loaded);
		CHECK(task() == EAssetLoadResult::Success);
	}
	LOAD_TEST("Deferred Load Failure")
	{
		TestAsset asset;
		AssetLoadTask task = AwaitAndReturn(AssetRef<TestAsset>{ &asset });
		CHECK(task() == EAssetLoadResult::TryAgain);
		CHECK(task() == EAssetLoadResult::TryAgain);
		asset.SetState(EAssetState::Invalid);
		CHECK(task() == EAssetLoadResult::Failure);
	}
} // namespace apollo::asset_ut