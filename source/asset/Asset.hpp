#pragma once

/** \file Asset.hpp */

#include <PCH.hpp>

#include <atomic>
#include <core/Enum.hpp>
#include <core/RetainPtr.hpp>
#include <core/ULID.hpp>

namespace apollo {

	struct AssetMetadata;

	/// Identifies a specific type of asset
	enum class EAssetType : int8
	{
		Invalid = -1,
		Texture2D,
		VertexShader,
		FragmentShader,
		Material,
		MaterialInstance,
		Mesh,
		FontAtlas,
		Scene,
		NTypes
	};

	/// These flags are used to specify the state an asset is currently in
	enum class EAssetState : uint8
	{
		Invalid = 0,			  /*!< Default state after creation */
		Loading = BIT(0),		  /*!< The asset is currently being loaded */
		LoadingDeferred = BIT(1), /*!< Used when the loading operation couldn't be completed in one
								   * go, and needs to be resumed */
		Loaded = BIT(2),		  /*!< The asset has been loaded successfully */
		Unloading = BIT(3),		  /*!< The asset is currently being unloaded */
		Unloaded = BIT(4),		  /*!< The asset has been unloaded */
		LoadingFailed = BIT(5),	  /*!< A loading attempt was performed and failed */
	};

	/**
	 * Converts a EAssetType into a readable string. Asserts if \p type is not a valid asset type
	 * \sa EAssetType
	 */
	[[nodiscard]] APOLLO_API std::string_view GetAssetTypeName(const EAssetType type) noexcept;

	/**
	 * \brief Abstract Asset interface. All asset classes implement this.
	 * \details This class uses an internal ref counter to manage the asset's lifetime.
	 * \sa \ref apollo::AssetRef "AssetRef"
	 * \sa IAssetManager
	 * \sa GET_ASSET_TYPE_IMPL
	 */
	class IAsset
	{
	public:
		/** \brief Constructs an asset with a randomly-generated ULID */
		APOLLO_API IAsset();
		/** \brief Constructs an asset from the provided ULID */
		IAsset(const ULID& id)
			: m_Id{ id }
		{}
		virtual ~IAsset() = default;

		[[nodiscard]] apollo::ULID GetId() const noexcept { return m_Id; }
		[[nodiscard]] EAssetState GetState() const noexcept { return m_State; }

		[[nodiscard]] bool IsLoading() const noexcept
		{
			return bool(m_State.load() & EAssetState::Loading);
		}
		[[nodiscard]] bool IsLoadingDeferred() const noexcept
		{
			return bool(m_State.load() & EAssetState::LoadingDeferred);
		}
		[[nodiscard]] bool IsLoaded() const noexcept
		{
			return bool(m_State.load() & EAssetState::Loaded);
		}

		[[nodiscard]] virtual APOLLO_API EAssetType GetType() const noexcept = 0;
		[[nodiscard]] APOLLO_API std::string_view GetTypeName() const noexcept;

	protected:
		apollo::ULID m_Id;
		std::atomic<EAssetState> m_State = EAssetState::Invalid;
		std::atomic_uint32_t m_RefCount = 0;

	private:
		void SetState(EAssetState state) noexcept { m_State = state; }

		friend class IAssetManager;
		friend struct AssetLoadRequest;
		friend struct AssetRetainTraits;
	};

/** \def GET_ASSET_TYPE_IMPL(type)
 * \brief Helper macro to implement the IAsset interface for a given \ref apollo::EAssetType
 * "EAssetType"
 * \details This is used in specific classes which derive from \ref apollo::IAsset "IAsset"
 */
#define GET_ASSET_TYPE_IMPL(type)                                                                  \
	static constexpr EAssetType AssetType = (type);                                                \
	[[nodiscard]] EAssetType GetType() const noexcept override                                     \
	{                                                                                              \
		return AssetType;                                                                          \
	}

	/**
	 * \brief Asset type requirements
	 * \tparam A: The asset class to check
	 * \details All asset classes must implement the IAsset interface:
	 * - \b A::AssetType must be an accessible static constexpr value of type EAssetType
	 * - \b a.GetType() must return \b A::AssetType for any object \e a of type <b>const A</b>
	 *
	 * The easiest way to implement this interface is through the GET_ASSET_TYPE_IMPL macro.
	 */
	template <class A>
	concept Asset = requires(A & lhs, A& rhs, const A& a)
	{
		{ std::is_base_of_v<IAsset, A> };
		{ sizeof(lhs) }->std::same_as<size_t>; // completeness test
		{ std::is_same_v<decltype(A::AssetType), EAssetType> };
		{ EAssetType::Invalid < A::AssetType && A::AssetType < EAssetType::NTypes };
		{ lhs.Swap(rhs) };
		{ a.GetType() }->std::same_as<EAssetType>;
	};

	/// Helper function to instanciate an asset of a given type
	template <Asset A>
	IAsset* ConstructAsset(const ULID& id) requires(std::is_base_of_v<IAsset, A>)
	{
		return static_cast<IAsset*>(new A{ id });
	};
} // namespace apollo