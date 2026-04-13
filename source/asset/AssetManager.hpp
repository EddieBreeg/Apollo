#pragma once

/** \file AssetManager.hpp */

#include <PCH.hpp>

#include "Asset.hpp"
#include "AssetFunctions.hpp"
#include "AssetLoader.hpp"
#include "AssetRef.hpp"
#include <core/ULID.hpp>

#include <memory>
#include <shared_mutex>
#include <string>

namespace apollo::rdr {
	class GPUDevice;
}

namespace apollo {
	/// Runtime information about a specific asset type
	struct AssetTypeInfo
	{
		AssetConstructor* m_Create = nullptr;
		AssetImportFunc* m_LoadFunc = nullptr;

		[[nodiscard]] operator bool() const noexcept { return m_Create && m_LoadFunc; }
	};

	/**
	 * \brief All relevant information about a specific Asset: ID, name etc
	 * \sa IAssetManager::ImportMetadataBank
	 */
	struct AssetMetadata
	{
		ULID m_Id;
		std::string m_Name;
		std::string m_FilePath;
		uint32 m_Offset = 0;
		EAssetType m_Type = EAssetType::Invalid;
	};

	/**
	 * \brief Handles all assets in the game
	 */
	class IAssetManager
	{
	public:
		/**
		 * \brief Destroys every asset in the cache before exiting
		 */
		virtual APOLLO_API ~IAssetManager();
		/**
		 * \brief Loads all asset metadata for a given project. The implementation is different in
		 * the editor vs the game app
		 */
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

		/// \anchor getasset
		/** \name GetAsset
		 * \brief Retrieves an asset from its ULID
		 * \tparam A: The destination type
		 * \param id: The asset's ID
		 * \param type: The asset's type
		 * \param callback: An optional callback to invoke when asset completes loading. If the
		 * asset is already loaded, the callback is invoked immediately
		 * \details This function first looks up the ULID in the internal cache. If the asset is
		 * found and has the correct type, it is returned immediately. Otherwise, the asset is
		 * created and added to the cache; then a load request is submitted to the \ref AssetLoader
		 * "asset loader"
		 * \retval AssetRef A reference to the specific asset if it exists, null otherwise
		 * \note If an asset exists with the given ULID, type checking will be performed to ensure
		 * we're actually retrieving the right type of asset. An assert will be raised if this check
		 * fails.
		 * @{ */

		/** \brief Returns a generic AssetRef
		 */
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

		template <Asset A, class F>
		AssetRef<A> GetAsset(const ULID& id, F&& callback) requires(std::is_invocable_v<F, IAsset&>)
		{
			IAsset* const ptr = GetAssetImpl(
				id,
				A::AssetType,
				UniqueFunction<void(IAsset&)>{ std::forward<F>(callback) });
			return AssetRef<A>{ static_cast<A*>(ptr) };
		}
		/** @} */

		/**
		 * \brief Manually creates a new asset and adds it to the internal cache.
		 * \note This function does not submit a load request.
		 */
		template <Asset A, class... Args>
		AssetRef<A> AddTempAsset(Args&&... args) requires(std::constructible_from<A, Args...>)
		{
			A* ptr = new A{ std::forward<Args>(args)... };
			m_Cache.emplace(ptr->GetId(), ptr);
			return AssetRef{ ptr };
		}

		/** \brief Retrieves the metadata for a given asset ID. Returns nullptr if the asset wasn't
		 * found in the bank */
		[[nodiscard]] APOLLO_API const AssetMetadata* GetAssetMetadata(
			const ULID& id) const noexcept;

		/**
		 * \brief Updates the asset loader and processes unload requests. Called every frame.
		 */
		APOLLO_API void Update();
		/**
		 * \brief Returns the project's root asset path. This is where all the asset data/metadata
		 * lives.
		 */
		const std::string& GetAssetPath() const noexcept { return m_AssetsPath; }

		[[nodiscard]] AssetLoader& GetAssetLoader() noexcept { return m_Loader; }

		APOLLO_API IAssetManager(
			const std::string& assetsPath,
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

		std::string m_AssetsPath;
		AssetLoader m_Loader;
		Queue<IAsset*> m_UnloadQueue;

		static APOLLO_API std::unique_ptr<IAssetManager> s_Instance;
	};

} // namespace apollo

namespace apollo::json {
	/** \name JSON visitors
	 * \brief These helpers allow to easily retrieve an asset reference from a ULID in JSON format
	 * @{ */
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
	/** @} */
} // namespace apollo::json