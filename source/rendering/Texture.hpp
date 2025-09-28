#pragma once

#include <PCH.hpp>
#include <asset/Asset.hpp>

#include "HandleWrapper.hpp"

struct SDL_GPUTexture;

namespace brk::editor {

	bool ImportTexture2d(IAsset& out_texture, const AssetMetadata& metadata);
}

namespace brk::rdr {
	enum class EPixelFormat : int8
	{
		Invalid = -1,

		// unsigned 8-bit formats
		R8_UNorm,
		RG8_UNorm,
		RGBA8_UNorm,

		// signed 8-bit formats
		R8_SNorm,
		RG8_SNorm,
		RGBA8_SNorm,

		NFormats
	};

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
		ETextureUsageFlags m_Usage = ETextureUsageFlags::GraphicsShaderRead;
	};

	class BRK_API Texture2D : public IAsset, public _internal::HandleWrapper<SDL_GPUTexture*>
	{
	public:
		using IAsset::IAsset;
		Texture2D(const ULID& id, const TextureSettings& settings);
		Texture2D(const TextureSettings& settings)
			: Texture2D(ULID::Generate(), settings)
		{}

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

		~Texture2D();

		[[nodiscard]] const TextureSettings& GetSettings() const noexcept { return m_Settings; }

		GET_ASSET_TYPE_IMPL(EAssetType::Texture2D);

	private:
		TextureSettings m_Settings;
		friend bool brk::editor::ImportTexture2d(IAsset&, const AssetMetadata&);
	};
} // namespace brk::rdr