#pragma once

#include <PCH.hpp>
#include <asset/Asset.hpp>

#include "HandleWrapper.hpp"
#include "Pixel.hpp"

struct SDL_GPUTexture;

namespace apollo {
	enum class EAssetLoadResult : int8;
}

namespace apollo::editor {
	template <class>
	struct AssetHelper;
}

namespace apollo::rdr {

	enum class ETextureUsageFlags
	{
		None = 0,
		Sampled = BIT(0),
		ColorTarget = BIT(1),
		DepthStencilTarget = BIT(2),
		GraphicsShaderRead = BIT(3),
		ComputeShaderRead = BIT(4),
		ComputeShaderWrite = BIT(5),
		ComputeShaderReadWrite = BIT(6),
		NFlags = 7
	};

	struct TextureSettings
	{
		uint32 m_Width = 0;
		uint32 m_Height = 0;
		EPixelFormat m_Format = EPixelFormat::RGBA8_UNorm;
		ETextureUsageFlags m_Usage = ETextureUsageFlags::Sampled;
	};

	class Texture2D : public IAsset, public _internal::HandleWrapper<SDL_GPUTexture*>
	{
	public:
		using IAsset::IAsset;
		Texture2D()
			: BaseType::BaseType()
		{}

		APOLLO_API Texture2D(const ULID& id, const TextureSettings& settings);
		Texture2D(const TextureSettings& settings)
			: Texture2D(ULID::Generate(), settings)
		{}
		Texture2D(Texture2D&& other) noexcept
			: BaseType::BaseType(std::move(other))
			, m_Settings(other.m_Settings)
		{
			other.m_Settings = TextureSettings{ .m_Usage = ETextureUsageFlags::None };
		}

		using BaseType::BaseType;
		Texture2D& operator=(Texture2D&& other)
		{
			Swap(other);
			return *this;
		}

		void Swap(Texture2D& other)
		{
			BaseType::Swap(other);
			std::swap(m_Settings, other.m_Settings);
		}

		APOLLO_API ~Texture2D();

		[[nodiscard]] const TextureSettings& GetSettings() const noexcept { return m_Settings; }

		GET_ASSET_TYPE_IMPL(EAssetType::Texture2D);

	private:
		TextureSettings m_Settings;
		friend struct editor::AssetHelper<Texture2D>;
	};
} // namespace apollo::rdr