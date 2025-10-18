#include "AssetManager.hpp"
#include <core/Assert.hpp>
#include <core/Log.hpp>
#include <core/ULIDFormatter.hpp>

#include "Scene.hpp"
#include <core/Json.hpp>
#include <rendering/Material.hpp>
#include <rendering/Shader.hpp>
#include <rendering/Texture.hpp>
#include <rendering/text/FontAtlas.hpp>

namespace {
	void ProcessUnloadRequests(
		apollo::Queue<apollo::IAsset*> queue,
		apollo::ULIDMap<apollo::IAsset*> cache,
		std::shared_mutex& mutex)
	{
		while (queue.GetSize())
		{
			std::unique_lock lock{ mutex };
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
	std::unique_ptr<AssetManager> AssetManager::s_Instance;

	AssetManager::~AssetManager()
	{
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

	AssetManager::AssetManager(const AssetManagerSettings& settings, rdr::GPUDevice& gpuDevice)
		: m_ImportBank(settings.m_MetadataImportFunc)
		, m_TypeInfo{ 
			{&ConstructAsset<rdr::Texture2D>, settings.m_LoadTexture2d},
			{&ConstructAsset<rdr::Shader>, settings.m_LoadShader},
			{&ConstructAsset<rdr::Material>, settings.m_LoadMaterial},
			{&ConstructAsset<rdr::txt::FontAtlas>, settings.m_LoadFont},
			{&ConstructAsset<Scene>, settings.m_LoadScene},
		 }
		, m_AssetsPath(settings.m_AssetPath)
		, m_Loader(gpuDevice)
	{}

	bool AssetManager::ImportMetadataBank()
	{
		DEBUG_CHECK(m_ImportBank)
		{
			APOLLO_LOG_CRITICAL(
				"Called ImportMetadataBank on asset manager but bank import function is nullptr");
			return false;
		}
		DEBUG_CHECK(!m_AssetsPath.empty())
		{
			APOLLO_LOG_CRITICAL(
				"Called ImportMetadataBank on asset manager but asset folder path is empty");
			return false;
		}

		APOLLO_LOG_INFO("Loading project assets metadata from {}", m_AssetsPath.string());
		m_ImportBank(m_AssetsPath, m_MetadataBank);
		return true;
	}

	IAsset* AssetManager::GetAssetImpl(const ULID& id, EAssetType type)
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
				const EAssetType actualType = it->second->GetType();
				DEBUG_CHECK(actualType == type)
				{
					APOLLO_LOG_ERROR(
						"Asset {} has type {} instead of the expected {}",
						id,
						GetAssetTypeName(actualType),
						GetAssetTypeName(type));
					return nullptr;
				}
				return it->second;
			}
		}

		const AssetTypeInfo& info = m_TypeInfo[int32(type)];

		DEBUG_CHECK(info)
		{
			APOLLO_LOG_CRITICAL("Asset type {} is not implemented!", int32(type));
			return nullptr;
		}
		const auto it = m_MetadataBank.find(id);
		if (it == m_MetadataBank.end())
		{
			APOLLO_LOG_ERROR("No asset found for id {}", id);
			return nullptr;
		}
		auto ptr = info.m_Create(it->second.m_Id);
		{
			std::unique_lock lock{ m_Mutex };
			m_Cache.emplace(id, ptr);
		}

		ptr->SetState(EAssetState::Loading);
		m_Loader.AddRequest(AssetRef<IAsset>{ ptr }, info.m_Import, it->second);
		return ptr;
	}

	void AssetManager::RequestUnload(IAsset* ptr)
	{
		ptr->SetState(EAssetState::Unloading);
		std::unique_lock lock{ m_Mutex };
		m_UnloadQueue.AddEmplace(ptr);
	}

	void AssetManager::Update()
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
		auto* assetManager = AssetManager::GetInstance();
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