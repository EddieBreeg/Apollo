#include "AssetManager.hpp"
#include <core/Assert.hpp>
#include <core/Log.hpp>
#include <core/ULIDFormatter.hpp>

#include <rendering/Material.hpp>
#include <rendering/Shader.hpp>
#include <rendering/Texture.hpp>
#include <rendering/text/FontAtlas.hpp>

namespace {
	void ProcessUnloadRequests(brk::Queue<brk::IAsset*> queue, brk::ULIDMap<brk::IAsset*> cache)
	{
		while (queue.GetSize())
		{
			brk::IAsset* ptr = queue.PopAndGetFront();
			if (ptr->GetState() != brk::EAssetState::Unloading)
				continue;

			const brk::ULID& id = ptr->GetId();
			DEBUG_CHECK(cache.erase(id))
			{
				BRK_LOG_WARN(
					"Asset {} was marked for unload but wasn't found in asset cache",
					ptr->GetId());
			}
			BRK_LOG_TRACE("Unloading asset {}", id);
			delete ptr;
		}
	}
} // namespace

namespace brk {
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
				BRK_LOG_TRACE("Unloading asset {}", id);
				delete asset;
			}
		}
	}

	AssetManager::AssetManager(const AssetManagerSettings& settings, rdr::GPUDevice& gpuDevice)
		: m_ImportBank(settings.m_MetadataImportFunc)
		, m_TypeInfo{ 
			{&ConstructAsset<rdr::Texture2D>, settings.m_ImportTexture2d},
			{&ConstructAsset<rdr::Shader>, settings.m_LoadShader},
			{&ConstructAsset<rdr::Material>, settings.m_LoadMaterial},
			{&ConstructAsset<rdr::txt::FontAtlas>, settings.m_LoadFont},
		 }
		, m_AssetsPath(settings.m_AssetPath)
		, m_Loader(gpuDevice)
	{}

	bool AssetManager::ImportMetadataBank()
	{
		DEBUG_CHECK(m_ImportBank)
		{
			BRK_LOG_CRITICAL(
				"Called ImportMetadataBank on asset manager but bank import function is nullptr");
			return false;
		}
		DEBUG_CHECK(!m_AssetsPath.empty())
		{
			BRK_LOG_CRITICAL(
				"Called ImportMetadataBank on asset manager but asset folder path is empty");
			return false;
		}

		BRK_LOG_INFO("Loading project assets metadata from {}", m_AssetsPath.string());
		m_ImportBank(m_AssetsPath, m_MetadataBank);
		return true;
	}

	IAsset* AssetManager::GetAssetImpl(const ULID& id, EAssetType type)
	{
		BRK_ASSERT(
			type < EAssetType::NTypes && type > EAssetType::Invalid,
			"Invalid asset type {}",
			int32(type));
		{
			const auto it = m_Cache.find(id);
			if (it != m_Cache.end())
			{
				const EAssetType actualType = it->second->GetType();
				DEBUG_CHECK(actualType == type)
				{
					BRK_LOG_ERROR(
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
			BRK_LOG_CRITICAL("Asset type {} is not implemented!", int32(type));
			return nullptr;
		}
		const auto it = m_MetadataBank.find(id);
		if (it == m_MetadataBank.end())
		{
			BRK_LOG_ERROR("No asset found for id {}", id);
			return nullptr;
		}
		auto ptr = info.m_Create(it->second.m_Id);
		m_Cache.emplace(id, ptr);

		ptr->SetState(EAssetState::Loading);
		m_Loader.AddRequest(AssetRef<IAsset>{ ptr }, info.m_Import, it->second);
		return ptr;
	}

	void AssetManager::RequestUnload(IAsset* ptr)
	{
		ptr->SetState(EAssetState::Unloading);
		m_UnloadQueue.AddEmplace(ptr);
	}

	void AssetManager::Update()
	{
		m_Loader.ProcessRequests();
		ProcessUnloadRequests(m_UnloadQueue, m_Cache);
	}
} // namespace brk