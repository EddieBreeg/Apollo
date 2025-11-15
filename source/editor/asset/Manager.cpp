#include "Manager.hpp"
#include "AssetHelper.hpp"
#include <core/Errno.hpp>
#include <core/Log.hpp>
#include <core/StringHash.hpp>
#include <core/ULIDFormatter.hpp>
#include <fstream>

#include <asset/Scene.hpp>
#include <rendering/Material.hpp>
#include <rendering/Mesh.hpp>
#include <rendering/Shader.hpp>
#include <rendering/Texture.hpp>
#include <rendering/text/FontAtlas.hpp>

namespace {
	const apollo::StringHashMap<apollo::EAssetType> g_AssetTypeMap{
		{ apollo::StringHash{ "texture2d" }, apollo::EAssetType::Texture2D },
		{ apollo::StringHash{ "vertexShader" }, apollo::EAssetType::VertexShader },
		{ apollo::StringHash{ "fragmentShader" }, apollo::EAssetType::FragmentShader },
		{ apollo::StringHash{ "material" }, apollo::EAssetType::Material },
		{ apollo::StringHash{ "materialInstance" }, apollo::EAssetType::MaterialInstance },
		{ apollo::StringHash{ "mesh" }, apollo::EAssetType::Mesh },
		{ apollo::StringHash{ "fontAtlas" }, apollo::EAssetType::FontAtlas },
		{ apollo::StringHash{ "scene" }, apollo::EAssetType::Scene },
	};

	struct Parser
	{
		std::string_view m_Line;
		std::string_view GetNext()
		{
			if (m_Line.empty())
				return {};

			size_t endPos = m_Line.npos;
			size_t suffixLen = 0;
			size_t prefixLen = 0;

			if (m_Line[0] == '"')
			{
				prefixLen = 1;
				endPos = m_Line.find('"', 1);
				if (endPos == m_Line.npos)
				{
					APOLLO_LOG_ERROR("CSV sequence is malformed: {}", m_Line);
					return (m_Line = {});
				}
				if (endPos == (m_Line.length() - 1))
				{
					suffixLen = 1;
				}
				else if (m_Line[endPos + 1] == ',')
				{
					APOLLO_LOG_ERROR("CSV sequence is malformed: {}", m_Line);
					return (m_Line = {});
				}
				else
				{
					suffixLen = 2;
				}
			}
			else
			{
				endPos = m_Line.find(',');
				suffixLen = endPos < m_Line.size();
			}

			std::string_view val = m_Line.substr(prefixLen, endPos - prefixLen);
			if (endPos < m_Line.length())
				m_Line.remove_prefix(std::min(endPos + suffixLen, m_Line.length()));
			else
				m_Line = {};
			return val;
		}

#define CHECK_VAL(val, msg)                                                                        \
	if ((val).empty())                                                                             \
	{                                                                                              \
		APOLLO_LOG_ERROR("Failed to parse asset metadata: " msg);                                  \
		return false;                                                                              \
	}

		bool ParseMetadata(
			apollo::AssetMetadata& out_metadata,
			const std::filesystem::path& assetRoot)
		{
			std::string_view val = GetNext();
			CHECK_VAL(val, "missing asset ULID");
			out_metadata.m_Id = apollo::ULID::FromString(val);
			if (!out_metadata.m_Id)
			{
				APOLLO_LOG_ERROR("Failed to parse asset metadata: invalid ULID {}", val);
				return false;
			}
			val = GetNext();
			CHECK_VAL(val, "missing asset type");
			{
				const auto it = g_AssetTypeMap.find(apollo::StringHash{ val });
				if (it == g_AssetTypeMap.end())
				{
					APOLLO_LOG_ERROR("Failed to parse asset metadata: invalid type {}", val);
					return false;
				}
				out_metadata.m_Type = it->second;
			}

			val = GetNext();
			CHECK_VAL(val, "missing asset name");
			out_metadata.m_Name = val;

			val = GetNext();
			CHECK_VAL(val, "missing asset path");
			out_metadata.m_FilePath = assetRoot / val;

			return true;
		}
	};
#undef CHECK_VAL

	template <apollo::Asset A>
	consteval apollo::AssetTypeInfo CreateTypeInfo()
	{
		return apollo::AssetTypeInfo{
			.m_Create = &apollo::ConstructAsset<A>,
			.m_Import = &apollo::editor::AssetHelper<A>::Load,
		};
	}

	constexpr apollo::AssetTypeInfo g_TypeInfo[] = {
		CreateTypeInfo<apollo::rdr::Texture2D>(),
		CreateTypeInfo<apollo::rdr::VertexShader>(),
		CreateTypeInfo<apollo::rdr::FragmentShader>(),
		CreateTypeInfo<apollo::rdr::Material>(),
		CreateTypeInfo<apollo::rdr::MaterialInstance>(),
		CreateTypeInfo<apollo::rdr::Mesh>(),
		CreateTypeInfo<apollo::rdr::txt::FontAtlas>(),
		CreateTypeInfo<apollo::Scene>(),
	};

	constexpr void (*g_SwapFunctions[])(apollo::IAsset&, apollo::IAsset&) = {
		&apollo::editor::AssetHelper<apollo::rdr::Texture2D>::Swap,
		&apollo::editor::AssetHelper<apollo::rdr::VertexShader>::Swap,
		&apollo::editor::AssetHelper<apollo::rdr::FragmentShader>::Swap,
		&apollo::editor::AssetHelper<apollo::rdr::Material>::Swap,
		&apollo::editor::AssetHelper<apollo::rdr::MaterialInstance>::Swap,
		&apollo::editor::AssetHelper<apollo::rdr::Mesh>::Swap,
		&apollo::editor::AssetHelper<apollo::rdr::txt::FontAtlas>::Swap,
		&apollo::editor::AssetHelper<apollo::Scene>::Swap,
	};

	static_assert(STATIC_ARRAY_SIZE(g_TypeInfo) == size_t(apollo::EAssetType::NTypes));
	static_assert(STATIC_ARRAY_SIZE(g_SwapFunctions) == size_t(apollo::EAssetType::NTypes));
} // namespace

namespace apollo::editor {
	AssetManager::AssetManager(
		const std::filesystem::path& path,
		rdr::GPUDevice& device,
		mt::ThreadPool& threadPool)
		: IAssetManager(path, device, threadPool)
	{}

	bool AssetManager::ImportMetadataBank()
	{
		DEBUG_CHECK(!m_AssetsPath.empty())
		{
			APOLLO_LOG_CRITICAL(
				"Called ImportMetadataBank on asset manager but asset folder path is empty");
			return false;
		}

		APOLLO_LOG_INFO("Loading project assets metadata from {}", m_AssetsPath.string());
		const std::filesystem::path filePath = m_AssetsPath / "metadata.csv";

		std::ifstream inFile{
			filePath,
			std::ios::binary,
		};
		std::string line;
		if (!inFile.is_open())
		{
			APOLLO_LOG_ERROR("Failed to load {}: {}", filePath.string(), GetErrnoMessage(errno));
			return false;
		}
		std::getline(inFile, line);
		for (; inFile; std::getline(inFile, line))
		{
			if (line.empty() || line[0] == '\r')
				continue;

			Parser parser{ line };
			if (line.back() == '\r')
				parser.m_Line.remove_suffix(1);

			AssetMetadata metadata;
			if (!parser.ParseMetadata(metadata, m_AssetsPath))
				continue;

			auto res = m_MetadataBank.try_emplace(metadata.m_Id, std::move(metadata));
			if (res.second)
				continue;
			APOLLO_LOG_ERROR(
				"ULID {} is present multiple times in the metadata file. Only the first asset will "
				"be registered",
				metadata.m_Id);
		}
		return true;
	}

	void AssetManager::RequestReload(IAsset& asset)
	{
		const EAssetType type = asset.GetType();
		APOLLO_ASSERT(
			EAssetType::Invalid < type && type < EAssetType::NTypes,
			"Invalid asset type {}",
			int32(type));

		DEBUG_CHECK(asset.IsLoaded())
		{
			APOLLO_LOG_WARN("Requested reload for asset {}, but asset is not loaded", asset.GetId());
			return;
		}

		if (const AssetMetadata* metadata = GetAssetMetadata(asset.GetId()))
		{
			const AssetTypeInfo& typeInfo = g_TypeInfo[size_t(type)];
			IAsset* tempAsset = typeInfo.m_Create(ULID::Generate());
			m_Cache.emplace(tempAsset->GetId(), tempAsset);

			SetAssetState(*tempAsset, EAssetState::Loading);
			SetAssetState(asset, EAssetState::Loading);

			m_Loader.AddRequest(
				AssetLoadRequest{
					.m_Asset = AssetRef{ tempAsset },
					.m_Import = typeInfo.m_Import,
					.m_Metadata = metadata,
					.m_Callback =
						UniqueFunction<void(IAsset&)>{
							[&asset, swap = g_SwapFunctions[size_t(type)]](IAsset& tempAsset)
							{
								if (tempAsset.IsLoaded())
									swap(asset, tempAsset);

								SetAssetState(asset, EAssetState::Loaded);
							},
						},
				});
		}
	}

	const AssetTypeInfo& AssetManager::GetTypeInfo(EAssetType type) const
	{
		return g_TypeInfo[size_t(type)];
	}

} // namespace apollo::editor