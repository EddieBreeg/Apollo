#pragma once

#include <PCH.hpp>

#include <memory>

namespace brk {
	struct AssetMetadata;

	enum class EAssetType : int8
	{
		Invalid = -1,
		Texture2D,
		NTypes
	};

	enum class EAssetState : uint8
	{
		Invalid = 0,
		Loading = BIT(0),
		Loaded = BIT(1),
	};

	class IAsset
	{
	public:
		IAsset(const AssetMetadata& metadata)
			: m_Metadata(metadata)
		{}

		[[nodiscard]] const AssetMetadata& GetMetadata() const noexcept { return m_Metadata; }
		[[nodiscard]] EAssetState GetState() const noexcept { return m_State; }

	protected:
		const AssetMetadata& m_Metadata;
		EAssetState m_State = EAssetState::Invalid;

	private:
	};

	template <class A>
	std::shared_ptr<IAsset> ConstructAsset(const AssetMetadata& metadata)
		requires(std::is_base_of_v<IAsset, A>)
	{
		return std::static_pointer_cast<IAsset>(std::make_shared<A>(metadata));
	};
} // namespace brk