#pragma once

#include <PCH.hpp>

#include "Asset.hpp"
#include "AssetLoader.hpp"
#include "Importer.hpp"
#include <core/Singleton.hpp>
#include <core/ULID.hpp>

#include <filesystem>
#include <memory>
#include <string>

namespace brk::rdr {
	class GPUDevice;
}

namespace brk {
	enum class EAssetType : int8;

	struct AssetTypeInfo
	{
		AssetConstructor* m_Create = nullptr;
		AssetImportFunc* m_Import = nullptr;

		[[nodiscard]] operator bool() const noexcept { return m_Create && m_Import; }
	};

	struct AssetManagerSettings
	{
		AssetBankImportFunc* m_MetadataImportFunc = nullptr;

		// Asset importers
		AssetImportFunc* m_ImportTexture2d = nullptr;

		std::filesystem::path m_AssetPath;
	};

	struct AssetMetadata
	{
		ULID m_Id;
		std::string m_Name;
		std::filesystem::path m_FilePath;
		uint32 m_Offset = 0;
		EAssetType m_Type = EAssetType::Invalid;
	};

	class BRK_API AssetManager : public Singleton<AssetManager>
	{
	public:
		~AssetManager() = default;
		bool ImportMetadataBank();

		std::shared_ptr<IAsset> GetAsset(const ULID& id, EAssetType type);

		template <class A>
		std::shared_ptr<A> GetAsset(const ULID& id) requires(
			std::is_base_of_v<IAsset, A>&& A::AssetType > EAssetType::Invalid &&
			A::AssetType < EAssetType::NTypes)
		{
			return std::static_pointer_cast<A>(GetAsset(id, A::AssetType));
		}

		void Update();

	private:
		AssetManager(const AssetManagerSettings& settings, rdr::GPUDevice& gpuDevice);
		friend class Singleton<AssetManager>;

		ULIDMap<AssetMetadata> m_MetadataBank;
		AssetBankImportFunc* m_ImportBank = nullptr;
		AssetTypeInfo m_TypeInfo[int32(EAssetType::NTypes)];
		std::filesystem::path m_AssetsPath;
		AssetLoader m_Loader;

		static std::unique_ptr<AssetManager> s_Instance;
		friend class Singleton<AssetManager>;
	};
} // namespace brk