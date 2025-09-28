#include "Texture.hpp"
#include "Renderer.hpp"
#include <SDL3/SDL_gpu.h>
#include <core/Enum.hpp>
#include <core/Log.hpp>

namespace {
	using namespace brk::enum_operators;

#define HAS_NO_BITS(flags, mask) (!((flags) & (mask)))
#define HAS_ANY_BIT(flags, mask) (((flags) & (mask)) != static_cast<decltype(flags)>(0))

#ifdef BRK_DEV
	bool ValidateSettings(const brk::rdr::TextureSettings& settings)
	{
		if (!settings.m_Width)
		{
			BRK_LOG_ERROR("Invalid texture settings: width cannot be 0");
			return false;
		}
		if (!settings.m_Height)
		{
			BRK_LOG_ERROR("Invalid texture settings: height cannot be 0");
			return false;
		}
		using brk::rdr::EPixelFormat;
		using brk::rdr::ETextureUsageFlags;
		constexpr ETextureUsageFlags allUsageFlags = ETextureUsageFlags(127);

		if (HAS_NO_BITS(settings.m_Usage, allUsageFlags))
		{
			BRK_LOG_ERROR("Invalid texture settings: no usage flag provided");
			return false;
		}
		else if (
			HAS_ANY_BIT(settings.m_Usage, ETextureUsageFlags::Sampled) &&
			HAS_ANY_BIT(
				settings.m_Usage,
				ETextureUsageFlags::ComputeShaderRead | ETextureUsageFlags::GraphicsShaderRead |
					ETextureUsageFlags::ComputeShaderReadWrite))
		{
			BRK_LOG_ERROR(
				"Invalid texture settings: ETextureUsageFlags::Sampled cannot be used along with "
				"other Read/Write flags");
			return false;
		}

		if (settings.m_Format <= EPixelFormat::Invalid ||
			settings.m_Format >= EPixelFormat::NFormats)
		{
			BRK_LOG_ERROR("Invalid texture settings: invalid format {}", int32(settings.m_Format));
			return false;
		}
		return true;
	}
#endif
	constexpr SDL_GPUTextureFormat g_Formats[]{
		SDL_GPU_TEXTUREFORMAT_R8_UNORM,		  SDL_GPU_TEXTUREFORMAT_R8G8_UNORM,
		SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, SDL_GPU_TEXTUREFORMAT_R8_SNORM,
		SDL_GPU_TEXTUREFORMAT_R8G8_SNORM,	  SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SNORM,
	};
} // namespace

namespace brk::rdr {
	Texture2D::Texture2D(const ULID& id, const TextureSettings& settings)
		: IAsset(id)
		, m_Settings(settings)
	{
		DEBUG_CHECK(ValidateSettings(m_Settings))
		{
			return;
		}
		auto* device = Renderer::GetInstance()->GetDevice().GetHandle();
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
			BRK_LOG_ERROR("Failed to create texture: {}", SDL_GetError());
		}
	}

	Texture2D::~Texture2D()
	{
		if (m_Handle)
			SDL_ReleaseGPUTexture(Renderer::GetInstance()->GetDevice().GetHandle(), m_Handle);
	}
} // namespace brk::rdr