#include "Text.hpp"
#include <SDL3/SDL_gpu.h>
#include <asset/AssetManager.hpp>
#include <core/App.hpp>
#include <core/Log.hpp>
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

	struct TestSystem
	{
		brk::Window& m_Window;
		brk::rdr::Renderer& m_Renderer;
		brk::rdr::Texture2D m_Texture;
		brk::AssetRef<brk::rdr::Material> m_Material;
		brk::rdr::Buffer m_VBuffer;
		SDL_GPUGraphicsPipeline* m_Pipeline = nullptr;
		SDL_GPUSampler* m_Sampler = nullptr;
		float m_Scale = 1.0f;
		float m_OutlineThickness = 0.0f;
		glm::mat4x4 m_CamMatrix = glm::identity<glm::mat4x4>();
		glm::mat4x4 m_ModelMatrix = glm::identity<glm::mat4x4>();
		char m_RenderedChar = 'a';
		FreetypeContext m_FTContext;

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

		void InitTexture(
			msdfgen::Shape& glyphShape,
			rdr::GPUDevice& device,
			SDL_GPUCopyPass* copyPass)
		{
			static constexpr float scale = 64;
			static constexpr uint32 padding = 4;
			static constexpr float pxRange = 12;
			const auto bounds = glyphShape.getBounds();
			const uint32 width = scale * (bounds.r - bounds.l) + 0.5 + 2 * padding;
			const uint32 height = scale * (bounds.t - bounds.b) + 0.5 + 2 * padding;
			const msdfgen::Vector2 offset{
				padding / scale - bounds.l,
				padding / scale - bounds.b,
			};

			m_Texture = rdr::Texture2D(
				rdr::TextureSettings{
					.m_Width = width,
					.m_Height = height,
				});
			using Pixel = rdr::RGBAPixel<uint8>;
			const SDL_GPUTransferBufferCreateInfo tBufferInfo{
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = uint32(width * height * sizeof(Pixel)),
			};
			SDL_GPUTransferBuffer* tBuffer = SDL_CreateGPUTransferBuffer(
				device.GetHandle(),
				&tBufferInfo);

			BRK_ASSERT(tBuffer, "Failed to create transfer buffer: {}", SDL_GetError());
			rdr::BitmapView view{
				(Pixel*)SDL_MapGPUTransferBuffer(device.GetHandle(), tBuffer, false),
				width,
				height,
			};

			{
				const msdfgen::Range range{ pxRange / Min(width, height) };
				const msdfgen::SDFTransformation transform{
					msdfgen::Projection{ scale, offset },
					range,
				};
				msdfgen::Bitmap<float, 3> msdf{ (int)width, (int)height, msdfgen::Y_DOWNWARD };
				msdfgen::generateMSDF(msdf, glyphShape, transform);
				for (uint32 j = 0; j < height; ++j)
				{
					for (uint32 i = 0; i < width; ++i)
					{
						const float* inPixel = msdf(i, j);
						view(i, j) = Pixel{
							uint8(255 * Clamp(inPixel[0], 0.0f, 1.0f) + 0.5f),
							uint8(255 * Clamp(inPixel[1], 0.0f, 1.0f) + 0.5f),
							uint8(255 * Clamp(inPixel[2], 0.0f, 1.0f) + 0.5f),
							255,
						};
					}
				}
			}

			SDL_UnmapGPUTransferBuffer(device.GetHandle(), tBuffer);

			const SDL_GPUTextureTransferInfo transferInfo{
				.transfer_buffer = tBuffer,
				.offset = 0,
				.pixels_per_row = m_Texture.GetSettings().m_Width,
				.rows_per_layer = m_Texture.GetSettings().m_Height,
			};
			const SDL_GPUTextureRegion dest{
				.texture = m_Texture.GetHandle(),
				.w = width,
				.h = height,
				.d = 1,
			};
			SDL_UploadToGPUTexture(copyPass, &transferInfo, &dest, false);
			SDL_ReleaseGPUTransferBuffer(device.GetHandle(), tBuffer);
		}

		TestSystem(
			brk::Window& window,
			brk::rdr::Renderer& renderer,
			FreetypeContext ftCtx,
			char ch = 'a')
			: m_Window(window)
			, m_Renderer(renderer)
			, m_VBuffer(rdr::EBufferFlags::Vertex, 4 * sizeof(Vertex2d))
			, m_RenderedChar(ch)
			, m_FTContext(std::move(ftCtx))
		{
			m_ModelMatrix = glm::identity<glm::mat4x4>();

			glm::uvec2 winSize = m_Window.GetSize();
			const float xMax = .5f * winSize.x / winSize.y;
			m_CamMatrix = glm::orthoRH(-xMax, xMax, -0.5f, 0.5f, 0.01f, 10.0f);
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
						.color_write_mask = 0b1111,
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
				.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
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

			msdfgen::Shape shape;
			if (!m_FTContext.LoadGlyph(m_RenderedChar, shape))
			{
				return;
			}
			BRK_LOG_TRACE("Successfully decomposed glyph '{}'", m_RenderedChar);
			InitTexture(shape, m_Renderer.GetDevice(), copyPass);
			const float texRatio = float(m_Texture.GetSettings().m_Width) /
								   m_Texture.GetSettings().m_Height;
			const Vertex2d vert[] = {
				Vertex2d{ { -0.5f * texRatio, -.5f }, { 0, 1 } },
				Vertex2d{ { 0.5f * texRatio, -.5f }, { 1, 1 } },
				Vertex2d{ { -0.5f * texRatio, .5f }, { 0, 0 } },
				Vertex2d{ { 0.5f * texRatio, .5f }, { 1, 0 } },
			};
			m_VBuffer.UploadData(copyPass, vert, sizeof(vert));
			SDL_EndGPUCopyPass(copyPass);
			SDL_SubmitGPUCommandBuffer(cmdBuffer);
		}

		void DisplayUi()
		{
			ImGui::Begin("Settings");
			if (ImGui::SliderFloat("Scale", &m_Scale, 0, 1))
			{
				m_ModelMatrix[0].x = m_ModelMatrix[1].y = m_ModelMatrix[2].z = m_Scale;
			}
			ImGui::SliderFloat("Outline Thickness", &m_OutlineThickness, 0, 0.5f);
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

			if (!swapchainTexture || !m_Pipeline || !m_Texture)
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
			SDL_PushGPUFragmentUniformData(mainCommandBuffer, 0, &m_OutlineThickness, sizeof(float));

			SDL_BindGPUGraphicsPipeline(renderPass, m_Pipeline);

			const SDL_GPUBufferBinding bufferBinding{
				.buffer = m_VBuffer.GetHandle(),
				.offset = 0,
			};
			SDL_BindGPUVertexBuffers(renderPass, 0, &bufferBinding, 1);

			const SDL_GPUTextureSamplerBinding binding{
				.texture = m_Texture.GetHandle(),
				.sampler = m_Sampler,
			};
			SDL_BindGPUFragmentSamplers(renderPass, 0, &binding, 1);

			SDL_DrawGPUPrimitives(renderPass, 4, 1, 0, 0);

			SDL_EndGPURenderPass(renderPass);

			DisplayUi();
		}
	};

	brk::EAppResult Init(const brk::EntryPoint& entry, brk::App& app)
	{
		spdlog::set_level(spdlog::level::trace);
		const std::string
			fontPath = (entry.m_AssetManagerSettings.m_AssetPath / "assets/fonts/arial.ttf").string();
		const char ch = entry.m_Args.size() > 2 ? entry.m_Args[2][0] : 'a';
		auto& renderer = *brk::rdr::Renderer::GetInstance();
		ImGui::SetCurrentContext(app.GetImGuiContext());
		auto& manager = *brk::ecs::Manager::GetInstance();
		manager.AddSystem<TestSystem>(
			app.GetMainWindow(),
			renderer,
			FreetypeContext{ fontPath.c_str() },
			ch);
		return EAppResult::Continue;
	}
} // namespace brk::demo

void brk::GetEntryPoint(EntryPoint& out_entry)
{
	out_entry.m_AppName = "Breakout Example";
	out_entry.m_OnInit = brk::demo::Init;
}