#include "Text.hpp"
#include <SDL3/SDL_gpu.h>
#include <asset/AssetManager.hpp>
#include <core/App.hpp>
#include <core/Log.hpp>
#include <core/Utf8.hpp>
#include <core/Window.hpp>
#include <ecs/Manager.hpp>
#include <entry/Entry.hpp>
#include <entt/entity/registry.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <imgui.h>
#include <msdfgen.h>
#include <rendering/Bitmap.hpp>
#include <rendering/Buffer.hpp>
#include <rendering/Device.hpp>
#include <rendering/Material.hpp>
#include <rendering/Pixel.hpp>
#include <rendering/Renderer.hpp>
#include <rendering/Texture.hpp>
#include <systems/InputEventComponents.hpp>

namespace ImGui {
	void ShowDemoWindow(bool* p_open);
}

namespace brk::demo {
	struct Vertex2d
	{
		float2 m_Position;
		float2 m_Uv;
	};
	constexpr SDL_GPUVertexAttribute g_VertexAttributes[] = {
		SDL_GPUVertexAttribute{ .location = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2 },
		SDL_GPUVertexAttribute{
			.location = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(Vertex2d, m_Uv),
		},
	};
	constexpr SDL_GPUVertexBufferDescription g_VBufferDescription{
		.slot = 0,
		.pitch = sizeof(Vertex2d),
		.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
	};

	GlyphRange ParseGlyphRange(std::string_view text, uint32* out_unicodeLen = nullptr)
	{
		brk::utf8::Decoder decoder{ text };
		GlyphRange range{ UINT32_MAX, 0 };

		uint32 len = 0;
		while (char32_t c = decoder.DecodeNext())
		{
			if (c == utf8::g_InvalidCodePoint)
				c = ' ';
			++len;

			range.m_First = Min(range.m_First, c);
			range.m_Last = Max(range.m_Last, c);
		}

		if (out_unicodeLen)
			*out_unicodeLen = len;
		return range;
	}

	constexpr const char* g_DefaultText =
		R"(The quick brown
fox jumps over
the lazy dog.)";

	struct TestSystem
	{
		brk::Window& m_Window;
		brk::rdr::Renderer& m_Renderer;
		brk::AssetRef<brk::rdr::Material> m_Material;
		brk::rdr::Buffer m_VBuffer, m_IBuffer;
		SDL_GPUGraphicsPipeline* m_Pipeline = nullptr;
		SDL_GPUSampler* m_Sampler = nullptr;

		float m_Scale = 1.0f;

		float m_AntiAliasing = 1.0f;
		float m_OutlineThickness = 0.0f;
		float m_Glow = 0.0f;
		float m_GlowFalloff = 0.0f;
		glm::mat4x4 m_CamMatrix = glm::identity<glm::mat4x4>();
		glm::mat4x4 m_ModelMatrix = glm::identity<glm::mat4x4>();
		std::string_view m_Text;
		uint32 m_UnicodeLen = 0;
		GlyphRange m_GlyphRange;
		FontAtlas m_Atlas;
		RectF m_TextBox = {};

		void ProcessWindowResize(entt::registry& world)
		{
			using namespace brk::inputs;
			const auto view = world.view<const WindowResizeEventComponent>();
			if (view->empty())
				return;

			const WindowResizeEventComponent& eventData = *view->begin();

			const float xMax = .5f * eventData.m_Width / eventData.m_Height;
			m_CamMatrix = glm::orthoRH(-xMax, xMax, -.5f, .5f, .01f, 10.f);
		}

		void InitTexture(SDL_GPUCopyPass* copyPass)
		{
			m_Atlas.LoadRange(copyPass, m_GlyphRange, 32, 10.0f, 1024, 4);
		}

		TestSystem(
			brk::Window& window,
			brk::rdr::Renderer& renderer,
			FontAtlas atlas,
			std::string_view text)
			: m_Window(window)
			, m_Renderer(renderer)
			, m_Text(text)
			, m_Atlas(std::move(atlas))
		{
			m_ModelMatrix = glm::identity<glm::mat4x4>();

			glm::uvec2 winSize = m_Window.GetSize();
			const float xMax = .5f * winSize.x / winSize.y;
			m_CamMatrix = glm::orthoRH(-xMax, xMax, -0.5f, 0.5f, 0.01f, 10.0f);
			m_GlyphRange = ParseGlyphRange(m_Text, &m_UnicodeLen);
		}

		~TestSystem()
		{
			auto* device = m_Renderer.GetDevice().GetHandle();
			SDL_ReleaseGPUSampler(device, m_Sampler);
			SDL_ReleaseGPUGraphicsPipeline(device, m_Pipeline);
		}

		void InitPipeline()
		{
			brk::rdr::GPUDevice& device = m_Renderer.GetDevice();
			const SDL_GPUColorTargetDescription targetDesc{
				.format = SDL_GetGPUSwapchainTextureFormat(device.GetHandle(), m_Window.GetHandle()),
				.blend_state =
					SDL_GPUColorTargetBlendState{
						.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
						.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
						.color_blend_op = SDL_GPU_BLENDOP_ADD,
						.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
						.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
						.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
						.color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G |
											SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A,
						.enable_blend = true,
					},
			};
			const SDL_GPUGraphicsPipelineCreateInfo pipelineDesc{
				.vertex_shader = m_Material->GetVertexShader()->GetHandle(),
				.fragment_shader = m_Material->GetFragmentShader()->GetHandle(),
				.vertex_input_state =
					SDL_GPUVertexInputState{
						.vertex_buffer_descriptions = &g_VBufferDescription,
						.num_vertex_buffers = 1,
						.vertex_attributes = g_VertexAttributes,
						.num_vertex_attributes = STATIC_ARRAY_SIZE(g_VertexAttributes),
					},
				.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
				.rasterizer_state =
					SDL_GPURasterizerState{
						.fill_mode = SDL_GPU_FILLMODE_FILL,
						.cull_mode = SDL_GPU_CULLMODE_NONE,
					},
				.multisample_state =
					SDL_GPUMultisampleState{
						.sample_count = SDL_GPU_SAMPLECOUNT_1,
					},
				.depth_stencil_state = SDL_GPUDepthStencilState{},
				.target_info = SDL_GPUGraphicsPipelineTargetInfo{ &targetDesc, 1 },
			};
			m_Pipeline = SDL_CreateGPUGraphicsPipeline(device.GetHandle(), &pipelineDesc);
			BRK_ASSERT(m_Pipeline, "Failed to create graphics pipeline");
		} // namespace

		void PostInit()
		{
			auto& device = m_Renderer.GetDevice();

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

			BRK_LOG_TRACE("Example Post-Init");
			auto* assetManager = AssetManager::GetInstance();
			BRK_ASSERT(assetManager, "Asset manager hasn't been initialized!");

			m_Material = assetManager->GetAsset<brk::rdr::Material>(
				"01K6841M7W2D1J00QKJHHBDJG5"_ulid);
			auto* cmdBuffer = SDL_AcquireGPUCommandBuffer(device.GetHandle());
			SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuffer);

			InitTexture(copyPass);
			constexpr float textScale = 0.2f;
			const rdr::TextureSettings settings = m_Atlas.GetTexture().GetSettings();
			const float2 uvScale{ settings.m_Width, settings.m_Height };
			std::vector<Vertex2d> vertices;
			vertices.reserve(4 * m_UnicodeLen);
			std::vector<uint32> indices;
			indices.reserve(6 * m_UnicodeLen);

			float2 pos{ 0, 0 };
			utf8::Decoder decoder{ m_Text };
			const float fontScale = 1.0f / m_Atlas.GetPixelSize();
			char32_t prev = 0;

			uint32 baseIndex = 0;

			while (char32_t c = decoder.DecodeNext())
			{
				const Glyph* glyph = m_Atlas.GetGlyph(c);
				if (c == '\n')
				{
					pos = float2{ 0, pos.y - textScale };
					prev = c;
					continue;
				}
				else if (!(glyph->m_UvRect.width && glyph->m_UvRect.height))
				{
					pos.x += textScale * glyph->m_Advance;
					prev = c;
					continue;
				}
				pos += textScale * m_Atlas.GetKerning(prev, c);

				const RectU32& uvRect = glyph->m_UvRect;
				const float2 glyphSize{
					fontScale * uvRect.width,
					fontScale * uvRect.height,
				};

				const Vertex2d bottomLeft{
					pos + textScale * glyph->m_Offset,
					float2{ uvRect.x, uvRect.y + uvRect.height } / uvScale,
				};
				const Vertex2d topLeft{
					bottomLeft.m_Position + textScale * float2{ 0, glyphSize.y },
					float2{ uvRect.x, uvRect.y } / uvScale,
				};
				const Vertex2d bottomRight{
					bottomLeft.m_Position + textScale * float2{ glyphSize.x, 0 },
					float2{ uvRect.x + uvRect.width, uvRect.y + uvRect.height } / uvScale,
				};
				const Vertex2d topRight{
					bottomLeft.m_Position + glyphSize * textScale,
					float2{ uvRect.x + uvRect.width, uvRect.y } / uvScale,
				};
				m_TextBox += RectF{
					bottomLeft.m_Position.x,
					bottomLeft.m_Position.y,
					bottomRight.m_Position.x - bottomLeft.m_Position.x,
					topLeft.m_Position.y - bottomLeft.m_Position.y,
				};

				vertices.emplace_back(topLeft);
				vertices.emplace_back(bottomLeft);
				vertices.emplace_back(bottomRight);
				vertices.emplace_back(topRight);
				indices.emplace_back(baseIndex);
				indices.emplace_back(baseIndex + 1);
				indices.emplace_back(baseIndex + 2);
				indices.emplace_back(baseIndex + 2);
				indices.emplace_back(baseIndex + 3);
				indices.emplace_back(baseIndex);
				pos.x += textScale * glyph->m_Advance;
				prev = c;
				baseIndex += 4;
			}
			const float2 offset{ -0.5f * m_TextBox.width, 0.5f * m_TextBox.height };
			for (Vertex2d& v : vertices)
				v.m_Position += offset;

			const uint32 vertexDataSize = vertices.size() * sizeof(Vertex2d);
			const uint32 indexDataSize = 4 * indices.size();
			m_VBuffer = rdr::Buffer(rdr::EBufferFlags::Vertex, vertexDataSize);
			m_IBuffer = rdr::Buffer(rdr::EBufferFlags::Index, indexDataSize);

			m_VBuffer.UploadData(copyPass, vertices.data(), vertexDataSize);
			m_IBuffer.UploadData(copyPass, indices.data(), indexDataSize);

			SDL_EndGPUCopyPass(copyPass);
			SDL_SubmitGPUCommandBuffer(cmdBuffer);
		}

		void DisplayUi()
		{
			ImGui::Begin("Settings");
			ImGui::SliderFloat("Anti-Aliasing Width", &m_AntiAliasing, 0.0f, 5.0f);
			if (ImGui::SliderFloat("Scale", &m_Scale, 0, 1))
			{
				m_ModelMatrix[0].x = m_ModelMatrix[1].y = m_ModelMatrix[2].z = m_Scale;
			}
			ImGui::SliderFloat("Outline Thickness", &m_OutlineThickness, 0, 0.5f);
			ImGui::SliderFloat("Glow", &m_Glow, 0, 10.0f);
			ImGui::SliderFloat("Glow Falloff", &m_GlowFalloff, 0, 10.0f);
			ImGui::End();
		}

		void Update(entt::registry& world, const brk::GameTime&)
		{
			if (!m_Window) [[unlikely]]
				return;

			auto* swapchainTexture = m_Renderer.GetSwapchainTexture();
			if (!m_Pipeline && m_Material->GetState() == brk::EAssetState::Loaded)
			{
				InitPipeline();
			}

			rdr::Texture2D& texture = m_Atlas.GetTexture();

			if (!swapchainTexture || !m_Pipeline || !texture)
				return;

			ProcessWindowResize(world);

			auto* mainCommandBuffer = m_Renderer.GetMainCommandBuffer();

			const SDL_GPUColorTargetInfo targetInfo{
				.texture = swapchainTexture,
				.clear_color = { .1f, .1f, .1f, 1 },
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_STORE,
				.cycle = true,
			};

			SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
				mainCommandBuffer,
				&targetInfo,
				1,
				nullptr);

			SDL_PushGPUVertexUniformData(
				mainCommandBuffer,
				0,
				&m_CamMatrix,
				2 * sizeof(glm::mat4x4));
			SDL_PushGPUFragmentUniformData(
				mainCommandBuffer,
				0,
				&m_AntiAliasing,
				4 * sizeof(float));

			SDL_BindGPUGraphicsPipeline(renderPass, m_Pipeline);

			const SDL_GPUBufferBinding vBufferBinding{
				.buffer = m_VBuffer.GetHandle(),
				.offset = 0,
			};
			const SDL_GPUBufferBinding iBufferBinding{
				.buffer = m_IBuffer.GetHandle(),
				.offset = 0,
			};
			SDL_BindGPUVertexBuffers(renderPass, 0, &vBufferBinding, 1);
			SDL_BindGPUIndexBuffer(renderPass, &iBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

			const SDL_GPUTextureSamplerBinding binding{
				.texture = texture.GetHandle(),
				.sampler = m_Sampler,
			};
			const uint32 indexCount = m_IBuffer.GetSize() / 4;
			SDL_BindGPUFragmentSamplers(renderPass, 0, &binding, 1);

			SDL_DrawGPUIndexedPrimitives(renderPass, indexCount, 1, 0, 0, 0);

			SDL_EndGPURenderPass(renderPass);

			DisplayUi();
		}
	};

	brk::EAppResult Init(const brk::EntryPoint& entry, brk::App& app)
	{
		spdlog::set_level(spdlog::level::trace);
		const std::span args = entry.m_Args;
		const std::string fontPath =
			args.size() > 2
				? args[2]
				: (entry.m_AssetManagerSettings.m_AssetPath / "assets/fonts/arial.ttf").string();

		const char* text = args.size() > 3 ? args[3] : g_DefaultText;

		auto& renderer = *brk::rdr::Renderer::GetInstance();
		ImGui::SetCurrentContext(app.GetImGuiContext());
		auto& manager = *brk::ecs::Manager::GetInstance();
		manager.AddSystem<TestSystem>(
			app.GetMainWindow(),
			renderer,
			FontAtlas{ fontPath.c_str() },
			text);
		return EAppResult::Continue;
	}
} // namespace brk::demo

void brk::GetEntryPoint(EntryPoint& out_entry)
{
	out_entry.m_AppName = "Breakout Example";
	out_entry.m_OnInit = brk::demo::Init;
}