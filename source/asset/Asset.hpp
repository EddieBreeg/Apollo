#pragma once

#include <PCH.hpp>

#include <atomic>
#include <core/RetainPtr.hpp>
#include <core/ULID.hpp>

namespace brk {

	struct AssetMetadata;

	enum class EAssetType : int8
	{
		Invalid = -1,
		Texture2D,
		Shader,
		Material,
		NTypes
	};

	enum class EAssetState : uint8
	{
		Invalid = 0,
		Loading = BIT(0),
		Loaded = BIT(1),
		Unloading = BIT(2),
		Unloaded = BIT(3),
	};

	[[nodiscard]] BRK_API std::string_view GetAssetTypeName(const EAssetType type) noexcept;

	class IAsset
	{
	public:
		BRK_API IAsset();
		IAsset(const ULID& id)
			: m_Id{ id }
		{}
		virtual ~IAsset() = default;

		[[nodiscard]] brk::ULID GetId() const noexcept { return m_Id; }
		[[nodiscard]] EAssetState GetState() const noexcept { return m_State; }
		void SetState(EAssetState state) noexcept { m_State = state; }

		[[nodiscard]] virtual BRK_API EAssetType GetType() const noexcept = 0;
		[[nodiscard]] BRK_API std::string_view GetTypeName() const noexcept;

	protected:
		brk::ULID m_Id;
		std::atomic<EAssetState> m_State = EAssetState::Invalid;
		std::atomic_uint32_t m_RefCount = 0;

		friend struct AssetRetainTraits;

	private:
	};

#define GET_ASSET_TYPE_IMPL(type)                                                                  \
	static constexpr EAssetType AssetType = (type);                                                \
	[[nodiscard]] EAssetType GetType() const noexcept override                                     \
	{                                                                                              \
		return AssetType;                                                                          \
	}

	template <class A>
	IAsset* ConstructAsset(const ULID& id) requires(std::is_base_of_v<IAsset, A>)
	{
		return static_cast<IAsset*>(new A{ id });
	};
} // namespace brk