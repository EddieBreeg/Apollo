#pragma once

#include <PCH.hpp>

#include "Asset.hpp"
#include "AssetFunctions.hpp"
#include "AssetLoader.hpp"
#include "AssetRef.hpp"
#include <core/Singleton.hpp>
#include <core/ULID.hpp>

#include <filesystem>
#include <memory>
#include <shared_mutex>
#include <string>

namespace apollo::rdr {
	class GPUDevice;
}

namespace apollo {
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
		AssetImportFunc* m_LoadTexture2d = nullptr;
		AssetImportFunc* m_LoadVertexShader = nullptr;
		AssetImportFunc* m_LoadFragmentShader = nullptr;
		AssetImportFunc* m_LoadMaterial = nullptr;
		AssetImportFunc* m_LoadMaterialInstance = nullptr;
		AssetImportFunc* m_LoadMesh = nullptr;
		AssetImportFunc* m_LoadFont = nullptr;
		AssetImportFunc* m_LoadScene = nullptr;

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

	class AssetManager : public Singleton<AssetManager>
	{
	public:
		APOLLO_API ~AssetManager();
		APOLLO_API bool ImportMetadataBank();

		AssetRef<IAsset> GetAsset(const ULID& id, EAssetType type)
		{
			return AssetRef<IAsset>{ GetAssetImpl(id, type) };
		}

		template <class F>
		AssetRef<IAsset> GetAsset(const ULID& id, EAssetType type, F&& callback)
			requires(std::invocable<F, IAsset&>)
		{
			return AssetRef<IAsset>{
				GetAssetImpl(id, type, UniqueFunction<void(IAsset&)>{ std::forward<F>(callback) }),
			};
		}

		template <Asset A>
		AssetRef<A> GetAsset(const ULID& id)
		{
			IAsset* const ptr = GetAssetImpl(id, A::AssetType);
			return AssetRef<A>{ static_cast<A*>(ptr) };
		}

		/*
		 * Retrieves an asset, with an optional callback to be invoked once the asset is loaded.
		 * If the asset is already loaded, the callback will be invoked immediately, otherwise
		 * there is no strong guarantee as to when this will happen.
		 */
		template <Asset A, class F>
		AssetRef<A> GetAsset(const ULID& id, F&& callback) requires(std::is_invocable_v<F, IAsset&>)
		{
			IAsset* const ptr = GetAssetImpl(
				id,
				A::AssetType,
				UniqueFunction<void(IAsset&)>{ std::forward<F>(callback) });
			return AssetRef<A>{ static_cast<A*>(ptr) };
		}

		template <Asset A, class... Args>
		AssetRef<A> AddTempAsset(Args&&... args) requires(std::constructible_from<A, Args...>)
		{
			A* ptr = new A{ std::forward<Args>(args)... };
			m_Cache.emplace(ptr->GetId(), ptr);
			return AssetRef{ ptr };
		}

		[[nodiscard]] APOLLO_API const AssetMetadata* GetAssetMetadata(
			const ULID& id) const noexcept;

		APOLLO_API void Update();
		const std::filesystem::path& GetAssetPath() const noexcept { return m_AssetsPath; }

		[[nodiscard]] AssetLoader& GetAssetLoader() noexcept { return m_Loader; }

	private:
		APOLLO_API AssetManager(
			const AssetManagerSettings& settings,
			rdr::GPUDevice& gpuDevice,
			mt::ThreadPool& threadPool);
		friend class Singleton<AssetManager>;
		friend struct AssetRetainTraits;

		APOLLO_API IAsset* GetAssetImpl(
			const ULID& id,
			EAssetType type,
			UniqueFunction<void(IAsset&)> cbk = {});
		APOLLO_API void RequestUnload(IAsset* res);

		ULIDMap<AssetMetadata> m_MetadataBank;
		AssetBankImportFunc* m_ImportBank = nullptr;
		AssetTypeInfo m_TypeInfo[int32(EAssetType::NTypes)];
		std::shared_mutex m_Mutex;
		ULIDMap<IAsset*> m_Cache;

		std::filesystem::path m_AssetsPath;
		AssetLoader m_Loader;
		Queue<IAsset*> m_UnloadQueue;

		static APOLLO_API std::unique_ptr<AssetManager> s_Instance;
		friend class Singleton<AssetManager>;
	};

} // namespace apollo

namespace apollo::json {
	APOLLO_API bool Visit(
		AssetRef<IAsset>& out_ref,
		EAssetType type,
		const nlohmann::json& json,
		std::string_view key,
		bool isOptional = false);

	template <class A>
	bool Visit(
		AssetRef<A>& out_ref,
		const nlohmann::json& json,
		std::string_view key,
		bool isOptional = false)
		requires(
			std::is_base_of_v<IAsset, A>&& A::AssetType > EAssetType::Invalid &&
			A::AssetType < EAssetType::NTypes)
	{
		AssetRef<IAsset> ref;
		if (!Visit(ref, A::AssetType, json, key, isOptional))
			return false;

		out_ref = StaticPointerCast<A>(std::move(ref));
		return true;
	}
} // namespace apollo::json