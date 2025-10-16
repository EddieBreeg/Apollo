#include "BatchRenderer.hpp"

#include <SDL3/SDL_gpu.h>
#include <core/Assert.hpp>
#include <core/Utf8.hpp>
#include <rendering/Device.hpp>

namespace apollo::rdr::txt {
	Renderer2d::~Renderer2d()
	{
		if (m_Sampler) [[likely]]
			SDL_ReleaseGPUSampler(m_Device->GetHandle(), m_Sampler);
	}

	void Renderer2d::Init(
		const rdr::GPUDevice& device,
		AssetRef<Material> material,
		uint32 batchSize)
	{
		DEBUG_CHECK(material->GetState() != EAssetState::Invalid)
		{
			APOLLO_LOG_ERROR("Invalid material passed to txt::Renderer2d::Init");
			return;
		}
		m_Material = std::move(material);
		m_Device = &device;
		m_BatchSize = batchSize;
		m_Quads.reserve(m_BatchSize);

		m_Buffer = Buffer(EBufferFlags::GraphicsStorage, batchSize * sizeof(GlyphQuad));

		const SDL_GPUSamplerCreateInfo samplerInfo{
			.min_filter = SDL_GPU_FILTER_LINEAR,
			.mag_filter = SDL_GPU_FILTER_LINEAR,
			.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
			.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
			.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
			.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		};
		m_Sampler = SDL_CreateGPUSampler(device.GetHandle(), &samplerInfo);
		APOLLO_ASSERT(m_Sampler, "Failed to create sampler");
	}

	void Renderer2d::StartFrame(SDL_GPUCommandBuffer* cmdBuffer)
	{
		m_CopyPass = SDL_BeginGPUCopyPass(cmdBuffer);
	}

	void Renderer2d::AddText(std::string_view str, float2 origin, EAnchorPoint anchor)
	{
		APOLLO_ASSERT(IsInitialized(), "Called AddText on uninitialized text renderer");
		if (!m_Font || m_Font->GetState() != EAssetState::Loaded)
		{
			APOLLO_LOG_ERROR("Called AddText on text renderer, but font is not ready");
			return;
		}
		m_Dirty = true;
		const rdr::TextureSettings settings = m_Font->GetTexture().GetSettings();

		const float2 uvScale{ 1.0f / settings.m_Width, 1.0f / settings.m_Height };

		utf8::Decoder decoder{ str };
		const float fontScale = 1.0f / m_Font->GetPixelSize();
		const rdr::txt::Glyph* prev = nullptr;
		RectF bounds{
			0,
			0,
			0,
			0,
		};

		float2 pos = {};

		while (char32_t c = decoder.DecodeNext())
		{
			const rdr::txt::Glyph* glyph = m_Font->GetGlyph(c);
			if (!glyph) [[unlikely]]
				continue;

			const uint32 width = glyph->m_Uv.GetWidth(), height = glyph->m_Uv.GetHeight();
			if (c == '\n')
			{
				pos = float2{ 0, pos.y - m_Style.m_Size * m_Style.m_LineSpacing };
				continue;
			}
			else if (!(width && height))
			{
				pos.x += m_Style.m_Size * glyph->m_Advance;
				continue;
			}
			if (prev && m_Style.m_Kerning)
				pos += m_Style.m_Size * m_Style.m_Kerning * m_Font->GetKerning(*prev, *glyph);

			const float2 glyphSize{
				fontScale * width,
				fontScale * height,
			};
			const float4 rect{
				pos + m_Style.m_Size * glyph->m_Offset,
				m_Style.m_Size * glyphSize,
			};

			bounds += RectF{
				rect.x,
				rect.y,
				rect.x + rect.z,
				rect.y + rect.w,
			};
			m_Quads.emplace_back(GlyphQuad{
					.m_Rect = rect,
					.m_Uv = {
						uvScale.x * glyph->m_Uv.x0,
						uvScale.y * glyph->m_Uv.y0,
						uvScale.x * width,
						uvScale.y * height,
					},
					.m_MainColor = m_Style.m_FgColor,
					.m_OutlineColor = m_Style.m_OutlineColor,
					.m_OutlineThickness = m_Style.m_OutlineThickness,
				});

			pos.x += m_Style.m_Size * glyph->m_Advance * m_Style.m_Tracking;
			prev = glyph;
		}

		float2 offset = {};
		switch (anchor)
		{
		case EAnchorPoint::Center:
			offset = origin - 0.5f * float2{ bounds.x0 + bounds.x1, bounds.y0 + bounds.y1 };
			break;
		case EAnchorPoint::TopLeft: offset = origin - float2{ bounds.x0, bounds.y1 }; break;
		default: break;
		}

		for (GlyphQuad& q : m_Quads)
		{
			q.m_Rect.x += offset.x;
			q.m_Rect.y += offset.y;
		}
	}

	void Renderer2d::StartRender()
	{
		APOLLO_ASSERT(IsInitialized(), "Called AddText on uninitialized text renderer");
		if (m_Dirty)
		{
			Upload();
			m_Dirty = false;
		}
		SDL_EndGPUCopyPass(m_CopyPass);
		m_CopyPass = nullptr;
	}

	void Renderer2d::Render(SDL_GPURenderPass* renderPass)
	{
		APOLLO_ASSERT(IsInitialized(), "Called AddText on uninitialized text renderer");
		if (m_Quads.empty() || !m_Font || m_Font->GetState() != EAssetState::Loaded ||
			m_Material->GetState() != EAssetState::Loaded)
			return;

		const SDL_GPUTextureSamplerBinding samplerBinding{
			.texture = m_Font->GetTexture().GetHandle(),
			.sampler = m_Sampler,
		};

		SDL_BindGPUGraphicsPipeline(
			renderPass,
			static_cast<SDL_GPUGraphicsPipeline*>(m_Material->GetHandle()));
		SDL_GPUBuffer* const temp = m_Buffer.GetHandle();
		SDL_BindGPUFragmentSamplers(renderPass, 0, &samplerBinding, 1);
		SDL_BindGPUVertexStorageBuffers(renderPass, 0, &temp, 1);
		SDL_DrawGPUPrimitives(renderPass, 4, (uint32)m_Quads.size(), 0, 0);
	}

	void Renderer2d::Upload()
	{
		if (m_Quads.size() > m_BatchSize)
		{
			m_BatchSize = (uint32)m_Quads.size();
			m_Buffer = Buffer(EBufferFlags::GraphicsStorage, m_BatchSize * sizeof(GlyphQuad));
		}
		m_Buffer.UploadData(m_CopyPass, m_Quads.data(), uint32(m_Quads.size() * sizeof(GlyphQuad)));
	}
} // namespace apollo::rdr::txt