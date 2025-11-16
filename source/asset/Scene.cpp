#include "Scene.hpp"
#include "AssetManager.hpp"
#include <core/Log.hpp>

namespace apollo {
	void Scene::ReloadDeferred(IAssetManager& assetManager)
	{
		DEBUG_CHECK(IsLoaded())
		{
			APOLLO_LOG_ERROR("Called ReloadDeferred on scene {}, which is not loaded", m_Id);
			return;
		}

		const AssetMetadata* metadata = assetManager.GetAssetMetadata(m_Id);
		DEBUG_CHECK(metadata)
		{
			return;
		}

		auto& loader = assetManager.GetAssetLoader();
		AssetRef temp = assetManager.AddTempAsset<Scene>(ULID::Generate());
		m_State = temp->m_State = EAssetState::Loading;

		loader.AddRequest(
			AssetLoadRequest{
				.m_Asset{ StaticPointerCast<IAsset>(std::move(temp)) },
				.m_Import = assetManager.GetTypeInfo<Scene>().m_Import,
				.m_Metadata = metadata,
				.m_Callback{
					[this](IAsset& temp)
					{
						Swap(static_cast<Scene&>(temp));
						m_State = temp.GetState();
					},
				},
			});
	}
} // namespace apollo