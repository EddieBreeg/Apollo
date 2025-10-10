#include "Asset.hpp"
#include <core/Assert.hpp>

namespace {
	constexpr std::string_view g_TypeNames[] = {
		"texture2d",
		"shader",
		"material",
		"fontAtlas",
	};

	static_assert(
		STATIC_ARRAY_SIZE(g_TypeNames) == size_t(brk::EAssetType::NTypes),
		"g_TypeNames' size doesn't match brk::EAssetType::NTypes");
} // namespace

namespace brk {
	std::string_view GetAssetTypeName(const EAssetType type) noexcept
	{
		DEBUG_CHECK((type > EAssetType::Invalid) && (type < EAssetType::NTypes))
		{
			BRK_LOG_ERROR("Called GetAssetTypeName with invalid type {}", (int)type);
		}
		return g_TypeNames[(int)type];
	}

	IAsset::IAsset()
		: m_Id(ULID::Generate())
	{}

	[[nodiscard]] std::string_view IAsset::GetTypeName() const noexcept
	{
		const EAssetType type = GetType();
		BRK_ASSERT(
			(type > EAssetType::Invalid) && (type < EAssetType::NTypes),
			"Invalid asset type {}",
			int(type));
		return g_TypeNames[(int)type];
	}
} // namespace brk