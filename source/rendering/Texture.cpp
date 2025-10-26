#include "Texture.hpp"
#include "Context.hpp"
#include <SDL3/SDL_gpu.h>
#include <core/Enum.hpp>
#include <core/Log.hpp>

namespace {
	using namespace apollo::enum_operators;

#define HAS_NO_BITS(flags, mask) (!((flags) & (mask)))
#define HAS_ANY_BIT(flags, mask) (((flags) & (mask)) != static_cast<decltype(flags)>(0))

	bool ValidateSettings(const apollo::rdr::TextureSettings& settings)
	{
		if (!settings.m_Width)
		{
			APOLLO_LOG_ERROR("Invalid texture settings: width cannot be 0");
			return false;
		}
		if (!settings.m_Height)
		{
			APOLLO_LOG_ERROR("Invalid texture settings: height cannot be 0");
			return false;
		}
		using apollo::rdr::EPixelFormat;
		using apollo::rdr::ETextureUsageFlags;
		constexpr ETextureUsageFlags allUsageFlags = ETextureUsageFlags(127);

		if (HAS_NO_BITS(settings.m_Usage, allUsageFlags))
		{
			APOLLO_LOG_ERROR("Invalid texture settings: no usage flag provided");
			return false;
		}
		else if (
			HAS_ANY_BIT(settings.m_Usage, ETextureUsageFlags::Sampled) &&
			HAS_ANY_BIT(
				settings.m_Usage,
				ETextureUsageFlags::ComputeShaderRead | ETextureUsageFlags::GraphicsShaderRead |
					ETextureUsageFlags::ComputeShaderReadWrite))
		{
			APOLLO_LOG_ERROR(
				"Invalid texture settings: ETextureUsageFlags::Sampled cannot be used along with "
				"other Read/Write flags");
			return false;
		}

		if (settings.m_Format <= EPixelFormat::Invalid ||
			settings.m_Format >= EPixelFormat::NFormats)
		{
			APOLLO_LOG_ERROR(
				"Invalid texture settings: invalid format {}",
				int32(settings.m_Format));
			return false;
		}
		return true;
	}

	constexpr SDL_GPUTextureFormat g_Formats[]{
		SDL_GPU_TEXTUREFORMAT_R8_UNORM,			 SDL_GPU_TEXTUREFORMAT_R8G8_UNORM,
		SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,	 SDL_GPU_TEXTUREFORMAT_R8_SNORM,
		SDL_GPU_TEXTUREFORMAT_R8G8_SNORM,		 SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SNORM,
		SDL_GPU_TEXTUREFORMAT_D16_UNORM,		 SDL_GPU_TEXTUREFORMAT_D24_UNORM,
		SDL_GPU_TEXTUREFORMAT_D32_FLOAT,		 SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
		SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT,
	};
} // namespace

namespace apollo::rdr {
	Texture2D::Texture2D(const ULID& id, const TextureSettings& settings)
		: IAsset(id)
		, m_Settings(settings)
	{
		DEBUG_CHECK(ValidateSettings(m_Settings))
		{
			return;
		}
		auto* device = Context::GetInstance()->GetDevice().GetHandle();
		const SDL_GPUTextureCreateInfo info{
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = g_Formats[ToUnderlying(settings.m_Format)],
			.usage = SDL_GPUTextureUsageFlags(settings.m_Usage),
			.width = settings.m_Width,
			.height = settings.m_Height,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.sample_count = SDL_GPU_SAMPLECOUNT_1,
		};
		m_Handle = SDL_CreateGPUTexture(device, &info);
		DEBUG_CHECK(m_Handle)
		{
			APOLLO_LOG_ERROR("Failed to create texture: {}", SDL_GetError());
		}
	}

	Texture2D::~Texture2D()
	{
		if (m_Handle)
			SDL_ReleaseGPUTexture(Context::GetInstance()->GetDevice().GetHandle(), m_Handle);
	}
} // namespace apollo::rdr