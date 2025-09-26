#pragma once

#include <PCH.hpp>

#include <core/ULID.hpp>
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
		IAsset();
		IAsset(const ULID& id)
			: m_Id{ id }
		{}

		[[nodiscard]] brk::ULID GetId() const noexcept { return m_Id; }
		[[nodiscard]] EAssetState GetState() const noexcept { return m_State; }

	protected:
		brk::ULID m_Id;
		EAssetState m_State = EAssetState::Invalid;

	private:
	};

	template <class A>
	std::shared_ptr<IAsset> ConstructAsset(const ULID& id)
		requires(std::is_base_of_v<IAsset, A>)
	{
		return std::static_pointer_cast<IAsset>(std::make_shared<A>(id));
	};
} // namespace brk