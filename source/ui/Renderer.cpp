#include "Renderer.hpp"
#include <SDL3/SDL_gpu.h>
#include <UiShaders.hpp>
#include <clay.h>
#include <core/Assert.hpp>
#include <core/Log.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <rendering/Context.hpp>
#include <rendering/RenderPass.hpp>
#include <rendering/Shader.hpp>

namespace {
	SDL_GPUGraphicsPipeline* CreatePipeline(
		SDL_GPUDevice* device,
		std::string_view vSource,
		std::string_view fSource,
		SDL_GPUTextureFormat fmt)
	{
		using namespace apollo::ulid_literal;
		static constexpr apollo::ULID id = "01KM3KVNNASVP3KNJNRJV3AKX3"_ulid;

		using namespace apollo::rdr;
		const auto vShader = VertexShader::CompileFromSource(id, vSource);
		const auto fShader = FragmentShader::CompileFromSource(id, fSource);
		APOLLO_ASSERT(vShader, "Failed to compile builtin vertex shader");
		APOLLO_ASSERT(fShader, "Failed to compile builtin fragment shader");
		const SDL_GPUColorTargetDescription targetDesc{
			.format = fmt,
			.blend_state =
				SDL_GPUColorTargetBlendState{
					.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
					.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					.color_blend_op = SDL_GPU_BLENDOP_ADD,
					.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
					.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
					.color_write_mask = 255,
					.enable_blend = true,
				},
		};

		const SDL_GPUGraphicsPipelineCreateInfo info{
			.vertex_shader = vShader.GetHandle(),
			.fragment_shader = fShader.GetHandle(),
			.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
			.rasterizer_state =
				SDL_GPURasterizerState{
					.fill_mode = SDL_GPU_FILLMODE_FILL,
					.cull_mode = SDL_GPU_CULLMODE_BACK,
					.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
				},
			.multisample_state = SDL_GPUMultisampleState{ .sample_count = SDL_GPU_SAMPLECOUNT_1 },
			.target_info =
				SDL_GPUGraphicsPipelineTargetInfo{
					.color_target_descriptions = &targetDesc,
					.num_color_targets = 1,
				},
		};
		return SDL_CreateGPUGraphicsPipeline(device, &info);
	}
} // namespace

namespace apollo::rdr::ui {
	Renderer Renderer::s_Instance;

	void Renderer::Init(Context& ctx, EPixelFormat fmt)
	{
		if (m_RectPipeline)
		{
			APOLLO_LOG_WARN("Called apollo::rdr::ui::Renderer::Init() twice");
			return;
		}

		m_RenderContext = &ctx;
		m_RectPipeline = CreatePipeline(
			ctx.GetDevice().GetHandle(),
			data::shaders::g_UiRectVert,
			data::shaders::g_UiRectFrag,
			static_cast<SDL_GPUTextureFormat>(fmt));
	}

	void Renderer::Reset()
	{
		// Forced destroy all GPU buffers
		m_Rectangles = {};
		m_Borders = {};

		if (!m_RectPipeline)
			return;

		SDL_ReleaseGPUGraphicsPipeline(m_RenderContext->GetDevice().GetHandle(), m_RectPipeline);
		m_RenderContext = nullptr;
		m_RectPipeline = nullptr;
	}

	void Renderer::StartFrame()
	{
		m_Rectangles.Clear();
		m_Borders.Clear();
	}

	bool Renderer::SetTargetSize(float2 size) noexcept
	{
		if (size == m_TargetSize)
			return false;

		m_ProjMatrix = glm::orthoRH(0.0f, size.x, size.y, 0.0f, 0.0f, 1.0f);
		return true;
	}

	void Renderer::EndFrame(std::span<const Clay_RenderCommand> commands)
	{
		if (commands.empty())
			return;

		auto* cmdBuffer = m_RenderContext->GetMainCommandBuffer();
		auto* copyPass = SDL_BeginGPUCopyPass(cmdBuffer);

		m_Rectangles.StartRecording();
		m_Borders.StartRecording();

		for (const auto& cmd : commands)
		{
			switch (cmd.commandType)
			{
			case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
			{
				if (!cmd.renderData.rectangle.backgroundColor.a)
					continue;

				const float maxRadius = Min(cmd.boundingBox.width, cmd.boundingBox.height) / 2.0f;
				m_Rectangles.Add(
					UiRect{
						.Rectangle =
							float4{
								cmd.boundingBox.x,
								cmd.boundingBox.y,
								cmd.boundingBox.width,
								cmd.boundingBox.height,
							},
						.BgColor =
							float4{
								cmd.renderData.rectangle.backgroundColor.r,
								cmd.renderData.rectangle.backgroundColor.g,
								cmd.renderData.rectangle.backgroundColor.b,
								cmd.renderData.rectangle.backgroundColor.a,
							},
						.CornerRadius =
							float4{
								Min(maxRadius, cmd.renderData.rectangle.cornerRadius.topLeft),
								Min(maxRadius, cmd.renderData.rectangle.cornerRadius.topRight),
								Min(maxRadius, cmd.renderData.rectangle.cornerRadius.bottomLeft),
								Min(maxRadius, cmd.renderData.rectangle.cornerRadius.bottomRight),
							},
					});
			}
			break;

			case CLAY_RENDER_COMMAND_TYPE_BORDER:
			{
				const float4 color{
					cmd.renderData.border.color.r,
					cmd.renderData.border.color.g,
					cmd.renderData.border.color.b,
					cmd.renderData.border.color.a,
				};
				if (!color.a)
					continue;

				const float maxRadius = Min(cmd.boundingBox.width, cmd.boundingBox.height) / 2.0f;

				m_Rectangles.Add(
					UiRect{
						.BgColor = color,
						.BorderColor = color,
						.BorderThickness =
							float4{
								cmd.renderData.border.width.left,
								cmd.renderData.border.width.right,
								cmd.renderData.border.width.top,
								cmd.renderData.border.width.bottom,
							},
						.CornerRadius =
							float4{
								Min(maxRadius, cmd.renderData.border.cornerRadius.topLeft),
								Min(maxRadius, cmd.renderData.border.cornerRadius.topRight),
								Min(maxRadius, cmd.renderData.border.cornerRadius.bottomLeft),
								Min(maxRadius, cmd.renderData.border.cornerRadius.bottomRight),
							},
					});
			}
			break;
			default: break;
			}
		}

		m_Rectangles.EndRecording(copyPass);
		m_Borders.EndRecording(copyPass);
		SDL_EndGPUCopyPass(copyPass);

		m_RenderContext->AddCustomCommand(
			[this](rdr::Context& context)
			{
				auto* renderPass = context.GetCurrentRenderPass();
				auto* commandBuffer = context.GetMainCommandBuffer();
				SDL_PushGPUVertexUniformData(commandBuffer, 0, &m_ProjMatrix, sizeof(m_ProjMatrix));
				SDL_BindGPUGraphicsPipeline(renderPass->GetHandle(), m_RectPipeline);

				if (const uint32 count = m_Rectangles.GetCount())
				{
					SDL_GPUBuffer* const buf = m_Rectangles.GetBuffer().GetHandle();
					SDL_BindGPUVertexStorageBuffers(renderPass->GetHandle(), 0, &buf, 1);
					SDL_DrawGPUPrimitives(renderPass->GetHandle(), 4, count, 0, 0);
				}
				if (const uint32 count = m_Borders.GetCount())
				{
					SDL_GPUBuffer* const buf = m_Borders.GetBuffer().GetHandle();
					SDL_BindGPUVertexStorageBuffers(renderPass->GetHandle(), 0, &buf, 1);
					SDL_DrawGPUPrimitives(renderPass->GetHandle(), 4, count, 0, 0);
				}
			});
	}
} // namespace apollo::rdr::ui