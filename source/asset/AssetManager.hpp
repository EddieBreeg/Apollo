#pragma once

#include <PCH.hpp>

#include "Asset.hpp"
#include "AssetFunctions.hpp"
#include "AssetLoader.hpp"
#include "AssetRef.hpp"
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

	struct AssetMetadata
	{
		ULID m_Id;
		std::string m_Name;
		std::filesystem::path m_FilePath;
		uint32 m_Offset = 0;
		EAssetType m_Type = EAssetType::Invalid;
	};

	class IAssetManager
	{
	public:
		virtual APOLLO_API ~IAssetManager();
		virtual bool ImportMetadataBank() = 0;

		template <class ManagerType, class... Args>
		static ManagerType& Init(Args&&... args) requires(
			std::is_base_of_v<IAssetManager, ManagerType>&&
				std::constructible_from<ManagerType, Args...>)
		{
			ManagerType* ptr = new ManagerType{ std::forward<Args>(args)... };
			s_Instance.reset(ptr);
			return *ptr;
		}

		[[nodiscard]] static IAssetManager* GetInstance() noexcept { return s_Instance.get(); }

		static void Shutdown() { s_Instance.reset(); }

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

		APOLLO_API IAssetManager(
			const std::filesystem::path& assetsPath,
			rdr::GPUDevice& gpuDevice,
			mt::ThreadPool& threadPool);

		template <Asset A>
		[[nodiscard]] const AssetTypeInfo& GetTypeInfo() const
		{
			return GetTypeInfo(A::AssetType);
		}

	protected:
		friend struct AssetRetainTraits;

		APOLLO_API IAsset* GetAssetImpl(
			const ULID& id,
			EAssetType type,
			UniqueFunction<void(IAsset&)> cbk = {});
		APOLLO_API void RequestUnload(IAsset* res);

		static void SetAssetState(IAsset& asset, EAssetState state) noexcept
		{
			asset.SetState(state);
		}

		virtual const AssetTypeInfo& GetTypeInfo(EAssetType type) const = 0;

		ULIDMap<AssetMetadata> m_MetadataBank;
		std::shared_mutex m_Mutex;
		ULIDMap<IAsset*> m_Cache;

		std::filesystem::path m_AssetsPath;
		AssetLoader m_Loader;
		Queue<IAsset*> m_UnloadQueue;

		static APOLLO_API std::unique_ptr<IAssetManager> s_Instance;
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