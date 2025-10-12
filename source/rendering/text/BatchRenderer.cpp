#include "BatchRenderer.hpp"

#include <SDL3/SDL_gpu.h>
#include <core/Assert.hpp>
#include <core/Utf8.hpp>
#include <rendering/Device.hpp>

namespace {
	SDL_GPUGraphicsPipeline* CreatePipeline(
		const brk::rdr::GPUDevice& device,
		const brk::rdr::Shader& vShader,
		const brk::rdr::Shader& fShader,
		const SDL_GPUColorTargetDescription& target)
	{
		const SDL_GPUGraphicsPipelineCreateInfo desc{
			.vertex_shader = vShader.GetHandle(),
			.fragment_shader = fShader.GetHandle(),
			.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
			.rasterizer_state =
				SDL_GPURasterizerState{
					.fill_mode = SDL_GPU_FILLMODE_FILL,
					.cull_mode = SDL_GPU_CULLMODE_BACK,
					.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
				},
			.target_info =
				SDL_GPUGraphicsPipelineTargetInfo{
					.color_target_descriptions = &target,
					.num_color_targets = 1,
				},
		};
		return SDL_CreateGPUGraphicsPipeline(device.GetHandle(), &desc);
	}
} // namespace

namespace brk::rdr::txt {
	Renderer2d::~Renderer2d()
	{
		if (m_Pipeline) [[likely]]
			SDL_ReleaseGPUGraphicsPipeline(m_Device->GetHandle(), m_Pipeline);
		if (m_Sampler) [[likely]]
			SDL_ReleaseGPUSampler(m_Device->GetHandle(), m_Sampler);
	}

	void Renderer2d::Init(
		const rdr::GPUDevice& device,
		const Shader& vertexShader,
		const Shader& fragmentShader,
		const SDL_GPUColorTargetDescription& targetDesc,
		uint32 batchSize)
	{
		m_Device = &device;
		BRK_ASSERT(
			vertexShader.GetState() == EAssetState::Loaded,
			"Text renderer init failed: vertex shader is not loaded");
		BRK_ASSERT(
			fragmentShader.GetState() == EAssetState::Loaded,
			"Text renderer init failed: fragment shader is not loaded");
		BRK_ASSERT(
			vertexShader.GetStage() == EShaderStage::Vertex,
			"Text renderer init failed: vertex shader is not a vertex shader");
		BRK_ASSERT(
			fragmentShader.GetStage() == EShaderStage::Fragment,
			"Text renderer init failed: fragment shader is not a vertex shader");
		m_Pipeline = CreatePipeline(device, vertexShader, fragmentShader, targetDesc);
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
		BRK_ASSERT(m_Sampler, "Failed to create sampler");
	}

	void Renderer2d::StartFrame(SDL_GPUCommandBuffer* cmdBuffer)
	{
		m_CopyPass = SDL_BeginGPUCopyPass(cmdBuffer);
	}

	void Renderer2d::AddText(std::string_view str, float2 origin, EAnchorPoint anchor)
	{
		if (!m_Font || m_Font->GetState() != EAssetState::Loaded)
		{
			BRK_LOG_ERROR("Called AddText on text renderer, but font is not ready");
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
				pos = float2{ 0, pos.y - m_Style.m_Size };
				continue;
			}
			else if (!(width && height))
			{
				pos.x += m_Style.m_Size * glyph->m_Advance;
				continue;
			}
			if (prev)
				pos += m_Style.m_Size * m_Font->GetKerning(*prev, *glyph);

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
		BRK_ASSERT(IsInitialized(), "Called EndFrame on uninitialised text renderer");
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
		BRK_ASSERT(IsInitialized(), "Called EndFrame on uninitialised text renderer");
		if (m_Quads.empty() || !m_Font || m_Font->GetState() != EAssetState::Loaded)
			return;

		const SDL_GPUTextureSamplerBinding samplerBinding{
			.texture = m_Font->GetTexture().GetHandle(),
			.sampler = m_Sampler,
		};

		SDL_BindGPUGraphicsPipeline(renderPass, m_Pipeline);
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
} // namespace brk::rdr::txt