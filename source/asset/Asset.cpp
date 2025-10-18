#include "Asset.hpp"
#include <core/Assert.hpp>

namespace {
	constexpr std::string_view g_TypeNames[] = {
		"texture2d",
		"shader",
		"material",
		"fontAtlas",
		"scene",
	};

	static_assert(
		STATIC_ARRAY_SIZE(g_TypeNames) == size_t(apollo::EAssetType::NTypes),
		"g_TypeNames' size doesn't match apollo::EAssetType::NTypes");
} // namespace

namespace apollo {
	std::string_view GetAssetTypeName(const EAssetType type) noexcept
	{
		DEBUG_CHECK((type > EAssetType::Invalid) && (type < EAssetType::NTypes))
		{
			APOLLO_LOG_ERROR("Called GetAssetTypeName with invalid type {}", (int)type);
		}
		return g_TypeNames[(int)type];
	}

	IAsset::IAsset()
		: m_Id(ULID::Generate())
	{}

	[[nodiscard]] std::string_view IAsset::GetTypeName() const noexcept
	{
		const EAssetType type = GetType();
		APOLLO_ASSERT(
			(type > EAssetType::Invalid) && (type < EAssetType::NTypes),
			"Invalid asset type {}",
			int(type));
		return g_TypeNames[(int)type];
	}
} // namespace apollo