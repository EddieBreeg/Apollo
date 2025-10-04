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
#include <rendering/Buffer.hpp>
#include <rendering/Device.hpp>
#include <rendering/Material.hpp>
#include <rendering/Renderer.hpp>
#include <rendering/Texture.hpp>
#include <systems/InputEventComponents.hpp>

namespace ImGui {
	void ShowDemoWindow(bool* p_open);
}

namespace {
	struct Vertex2d
	{
		float2 m_Position;
		float2 m_Uv;
	};
	constexpr Vertex2d g_QuadVert[] = {
		Vertex2d{ { -.5f, -.5f }, { 0, 1 } },
		Vertex2d{ { .5f, -.5f }, { 1, 1 } },
		Vertex2d{ { -.5f, .5f }, { 0, 0 } },
		Vertex2d{ { .5f, .5f }, { 1, 0 } },
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
		brk::AssetRef<brk::rdr::Texture2D> m_Texture;
		brk::AssetRef<brk::rdr::Material> m_Material;
		brk::rdr::Buffer m_VBuffer;
		SDL_GPUGraphicsPipeline* m_Pipeline = nullptr;
		SDL_GPUSampler* m_Sampler = nullptr;
		glm::mat4x4 m_CamMatrix = glm::identity<glm::mat4x4>();

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

		TestSystem(brk::Window& window, brk::rdr::Renderer& renderer)
			: m_Window(window)
			, m_Renderer(renderer)
			, m_VBuffer(brk::rdr::EBufferFlags::Vertex, sizeof(g_QuadVert))
		{
			auto& device = m_Renderer.GetDevice();

			auto* cmdBuffer = SDL_AcquireGPUCommandBuffer(device.GetHandle());
			SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuffer);
			m_VBuffer.UploadData(copyPass, g_QuadVert, sizeof(g_QuadVert));
			SDL_EndGPUCopyPass(copyPass);
			SDL_SubmitGPUCommandBuffer(cmdBuffer);

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
				.blend_state = SDL_GPUColorTargetBlendState{},
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
		}

		void PostInit()
		{
			using namespace brk;

			BRK_LOG_TRACE("Example Post-Init");
			auto* assetManager = AssetManager::GetInstance();
			BRK_ASSERT(assetManager, "Asset manager hasb't been initialized!");

			m_Texture = assetManager->GetAsset<rdr::Texture2D>("01K61BK2C2QZS0A18PWHGARASK"_ulid);
			m_Material = assetManager->GetAsset<brk::rdr::Material>(
				"01K6841M7W2D1J00QKJHHBDJG5"_ulid);
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

			if (!swapchainTexture || !m_Pipeline)
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

			SDL_PushGPUVertexUniformData(mainCommandBuffer, 0, &m_CamMatrix, sizeof(m_CamMatrix));

			SDL_BindGPUGraphicsPipeline(renderPass, m_Pipeline);

			const SDL_GPUBufferBinding bufferBinding{
				.buffer = m_VBuffer.GetHandle(),
				.offset = 0,
			};
			SDL_BindGPUVertexBuffers(renderPass, 0, &bufferBinding, 1);

			const SDL_GPUTextureSamplerBinding binding{
				.texture = m_Texture->GetHandle(),
				.sampler = m_Sampler,
			};
			SDL_BindGPUFragmentSamplers(renderPass, 0, &binding, 1);

			SDL_DrawGPUPrimitives(renderPass, 4, 1, 0, 0);

			SDL_EndGPURenderPass(renderPass);

			ImGui::ShowDemoWindow(nullptr);
		}
	};

	brk::EAppResult Init(const brk::EntryPoint&, brk::App& app)
	{
		spdlog::set_level(spdlog::level::trace);
		auto& renderer = *brk::rdr::Renderer::GetInstance();
		ImGui::SetCurrentContext(app.GetImGuiContext());
		auto& manager = *brk::ecs::Manager::GetInstance();
		manager.AddSystem<TestSystem>(app.GetMainWindow(), renderer);
		return brk::EAppResult::Continue;
	}
} // namespace

void brk::GetEntryPoint(EntryPoint& out_entry)
{
	out_entry.m_AppName = "Breakout Example";
	out_entry.m_OnInit = Init;
}