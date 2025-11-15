#include "AssetManager.hpp"
#include <core/Assert.hpp>
#include <core/Json.hpp>
#include <core/Log.hpp>

namespace {
	void ProcessUnloadRequests(
		apollo::Queue<apollo::IAsset*>& queue,
		apollo::ULIDMap<apollo::IAsset*>& cache,
		std::shared_mutex& mutex)
	{
		for (;;)
		{
			std::unique_lock lock{ mutex };
			if (!queue.GetSize())
				return;

			apollo::IAsset* ptr = queue.PopAndGetFront();

			if (ptr->GetState() != apollo::EAssetState::Unloading)
				continue;

			const apollo::ULID& id = ptr->GetId();
			DEBUG_CHECK(cache.erase(id))
			{
				APOLLO_LOG_WARN(
					"Asset {} was marked for unload but wasn't found in asset cache",
					ptr->GetId());
			}
			lock.unlock();
			APOLLO_LOG_TRACE("Unloading asset {}", id);
			delete ptr;
		}
	}
} // namespace

namespace apollo {
	std::unique_ptr<IAssetManager> IAssetManager::s_Instance;

	IAssetManager::~IAssetManager()
	{
		m_Loader.Clear();

		while (m_Cache.size())
		{
			auto it = m_Cache.begin();
			while (it != m_Cache.end())
			{
				if (AssetRetainTraits::GetCount(it->second)) // asset still in use, delete later
				{
					++it;
					continue;
				}

				const auto [id, asset] = *it;
				it = m_Cache.erase(it);
				APOLLO_LOG_TRACE("Unloading asset {}", id);
				delete asset;
			}
		}
	}

	IAssetManager::IAssetManager(
		const std::filesystem::path& assetPath,
		rdr::GPUDevice& gpuDevice,
		mt::ThreadPool& tp)
		: m_AssetsPath(assetPath)
		, m_Loader(gpuDevice, tp)
	{}

	IAsset* IAssetManager::GetAssetImpl(
		const ULID& id,
		EAssetType type,
		UniqueFunction<void(IAsset&)> cbk)
	{
		APOLLO_ASSERT(
			type < EAssetType::NTypes && type > EAssetType::Invalid,
			"Invalid asset type {}",
			int32(type));
		{
			std::shared_lock lock{ m_Mutex };
			const auto it = m_Cache.find(id);
			if (it != m_Cache.end())
			{
				IAsset* const asset = it->second;
				const EAssetType actualType = asset->GetType();
				DEBUG_CHECK(actualType == type)
				{
					APOLLO_LOG_ERROR(
						"Asset {} has type {} instead of the expected {}",
						id,
						GetAssetTypeName(actualType),
						GetAssetTypeName(type));
					return nullptr;
				}

				if (cbk)
				{
					if (asset->IsLoading())
					{
						m_Loader.AddRequest(
							AssetLoadRequest{
								.m_Asset = AssetRef{ asset },
								.m_Callback = std::move(cbk),
							});
					}
					else
					{
						cbk(*asset);
					}
				}

				return asset;
			}
		}

		const AssetTypeInfo& info = GetTypeInfo(type);

		DEBUG_CHECK(info)
		{
			APOLLO_LOG_CRITICAL("Asset type {} is not implemented!", int32(type));
			return nullptr;
		}
		const auto* metadata = GetAssetMetadata(id);
		if (!metadata)
		{
			APOLLO_LOG_ERROR("No asset found for id {}", id);
			return nullptr;
		}
		auto ptr = info.m_Create(id);
		{
			std::unique_lock lock{ m_Mutex };
			m_Cache.emplace(id, ptr);
		}

		ptr->SetState(EAssetState::Loading);
		m_Loader.AddRequest(
			AssetLoadRequest{
				AssetRef<IAsset>{ ptr },
				info.m_Import,
				metadata,
				std::move(cbk),
			});
		return ptr;
	}

	const AssetMetadata* IAssetManager::GetAssetMetadata(const ULID& id) const noexcept
	{
		if (const auto it = m_MetadataBank.find(id); it != m_MetadataBank.end())
			return &it->second;

		APOLLO_LOG_ERROR("Couldn't find metadata for asset {}", id);
		return nullptr;
	}

	void IAssetManager::RequestUnload(IAsset* ptr)
	{
		ptr->SetState(EAssetState::Unloading);
		std::unique_lock lock{ m_Mutex };
		m_UnloadQueue.AddEmplace(ptr);
	}

	void IAssetManager::Update()
	{
		m_Loader.ProcessRequests();
		ProcessUnloadRequests(m_UnloadQueue, m_Cache, m_Mutex);
	}

	bool json::Visit(
		AssetRef<IAsset>& out_ref,
		EAssetType type,
		const nlohmann::json& json,
		std::string_view key,
		bool isOptional)
	{
		auto* assetManager = IAssetManager::GetInstance();
		APOLLO_ASSERT(assetManager, "Asset manager has not been initialized");
		ULID id;
		const auto it = json.find(key);

		if (it == json.end())
			return isOptional;
		if (!id.FromJson(*it))
			return false;

		return (out_ref = assetManager->GetAsset(id, type));
	}
} // namespace apollo