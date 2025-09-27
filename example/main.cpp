#include <SDL3/SDL_gpu.h>
#include <asset/AssetManager.hpp>
#include <core/App.hpp>
#include <core/Log.hpp>
#include <core/Window.hpp>
#include <ecs/Manager.hpp>
#include <entry/Entry.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <imgui.h>
#include <rendering/Device.hpp>
#include <rendering/Renderer.hpp>
#include <rendering/Shader.hpp>
#include <rendering/Texture.hpp>

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
		brk::AssetRef<brk::rdr::Shader> m_VShader, m_FShader;
		SDL_GPUBuffer* m_VBuffer = nullptr;
		SDL_GPUGraphicsPipeline* m_Pipeline = nullptr;
		SDL_GPUSampler* m_Sampler = nullptr;
		glm::mat4x4 m_CamMatrix = glm::identity<glm::mat4x4>();

		TestSystem(brk::Window& window, brk::rdr::Renderer& renderer)
			: m_Window(window)
			, m_Renderer(renderer)
		{
			auto& device = m_Renderer.GetDevice();
			SDL_GPUBufferCreateInfo bufCreateInfo{
				.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
				.size = sizeof(g_QuadVert),
			};
			m_VBuffer = SDL_CreateGPUBuffer(device.GetHandle(), &bufCreateInfo);
			BRK_ASSERT(m_VBuffer, "Failed to create vertex buffer");

			auto* cmdBuffer = SDL_AcquireGPUCommandBuffer(device.GetHandle());
			SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuffer);

			SDL_GPUTransferBufferCreateInfo transferBufferInfo{
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = sizeof(g_QuadVert)
			};
			auto* transferBuffer = SDL_CreateGPUTransferBuffer(
				device.GetHandle(),
				&transferBufferInfo);
			BRK_ASSERT(transferBuffer, "Failed to create transfer buffer");
			Vertex2d* bufMem = (Vertex2d*)
				SDL_MapGPUTransferBuffer(device.GetHandle(), transferBuffer, false);
			std::copy(std::begin(g_QuadVert), std::end(g_QuadVert), bufMem);

			SDL_UnmapGPUTransferBuffer(device.GetHandle(), transferBuffer);

			const SDL_GPUTransferBufferLocation loc{
				.transfer_buffer = transferBuffer,
			};
			const SDL_GPUBufferRegion dest{
				.buffer = m_VBuffer,
				.size = sizeof(g_QuadVert),
			};
			SDL_UploadToGPUBuffer(copyPass, &loc, &dest, false);
			SDL_ReleaseGPUTransferBuffer(device.GetHandle(), transferBuffer);
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
		}

		~TestSystem()
		{
			auto* device = m_Renderer.GetDevice().GetHandle();
			SDL_ReleaseGPUBuffer(device, m_VBuffer);
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
				.vertex_shader = m_VShader->GetHandle(),
				.fragment_shader = m_FShader->GetHandle(),
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

		void Update()
		{
			if (!m_Window) [[unlikely]]
				return;

			static bool firstFrame = true;
			if (firstFrame)
			{
				using namespace brk;

				firstFrame = false;
				auto* assetManager = AssetManager::GetInstance();
				BRK_ASSERT(assetManager, "Asset manager hasb't been initialized!");

				m_Texture = assetManager->GetAsset<rdr::Texture2D>(
					"01K61BK2C2QZS0A18PWHGARASK"_ulid);
				m_VShader = assetManager->GetAsset<brk::rdr::Shader>(
					"01K65F71RK1WNJ6D7XDNTNPVX4"_ulid);
				m_FShader = assetManager->GetAsset<brk::rdr::Shader>(
					"01K65MTCW215MP1KK72CG1WQ1T"_ulid);
			}

			auto* swapchainTexture = m_Renderer.GetSwapchainTexture();
			if (!m_Pipeline && m_VShader->GetState() == brk::EAssetState::Loaded &&
				m_FShader->GetState() == brk::EAssetState::Loaded)
			{
				InitPipeline();
			}

			if (!swapchainTexture || !m_Pipeline)
				return;

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

			const SDL_GPUBufferBinding bufferBinding{ .buffer = m_VBuffer, .offset = 0 };
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