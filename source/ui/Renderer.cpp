#include "Renderer.hpp"
#include <SDL3/SDL_gpu.h>
#include <clay.h>
#include <core/Assert.hpp>
#include <core/Log.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <rendering/Context.hpp>
#include <rendering/RenderPass.hpp>
#include <rendering/Shader.hpp>
#include <tools/ShaderCompiler.hpp>

#include <ui/SlangUiBorder.hpp>
#include <ui/SlangUiRect.hpp>

namespace {
	template <class V>
	struct VertexTypeStage;
	template <>
	struct VertexTypeStage<apollo::rdr::VertexShader>
	{
		static constexpr SlangStage Value = SLANG_STAGE_VERTEX;
		static constexpr const char Name[] = "vertex";
	};
	template <>
	struct VertexTypeStage<apollo::rdr::FragmentShader>
	{
		static constexpr SlangStage Value = SLANG_STAGE_FRAGMENT;
		static constexpr const char Name[] = "fragment";
	};

	template <class V>
	V CreateShaderFromSlangModule(
		const apollo::ULID& id,
		slang::IModule& module,
		const char* entryPoint,
		apollo::rdr::ShaderCompiler& compiler)
	{
		Slang::ComPtr<slang::IBlob> diagnostics;
		Slang::ComPtr code = compiler.GetTargetCode(
			module,
			entryPoint,
			VertexTypeStage<V>::Value,
			diagnostics.writeRef());
		DEBUG_CHECK(code)
		{
			APOLLO_LOG_CRITICAL(
				"Failed to link built-in {} shader: {:.{}}",
				VertexTypeStage<V>::Name,
				static_cast<const char*>(diagnostics->getBufferPointer()),
				diagnostics->getBufferSize());
			DEBUG_BREAK();
		}
		return V{ id, code->getBufferPointer(), code->getBufferSize() };
	}

	SDL_GPUGraphicsPipeline* CreatePipeline(
		SDL_GPUDevice* device,
		const apollo::ULID& id1,
		const apollo::ULID& id2,
		slang::IModule& module,
		SDL_GPUTextureFormat fmt,
		apollo::rdr::ShaderCompiler& compiler)
	{
		using namespace apollo::ulid_literal;

		using namespace apollo::rdr;

		const auto vShader = CreateShaderFromSlangModule<VertexShader>(
			id1,
			module,
			"vs_main",
			compiler);
		const auto fShader = CreateShaderFromSlangModule<FragmentShader>(
			id2,
			module,
			"fs_main",
			compiler);
		const SDL_GPUColorTargetDescription targetDesc{
			.format = fmt,
			.blend_state =
				SDL_GPUColorTargetBlendState{
					.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
					.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					.color_blend_op = SDL_GPU_BLENDOP_ADD,
					.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
					.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
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

	struct DrawData
	{
		using DrawCall = apollo::rdr::DrawCall;
		using RectangleBatch = apollo::rdr::Batch<apollo::rdr::ui::UiRect>;
		using BorderBatch = apollo::rdr::Batch<apollo::rdr::ui::Border>;

		DrawData(RectangleBatch& rectangleData, BorderBatch& borderData)
			: m_RectData(rectangleData)
			, m_BorderData(borderData)
			, m_Rectangles{ .m_NumVertices = 4, .m_NumInstances = 0 }
			, m_Borders{ .m_NumVertices = 4, .m_NumInstances = 0 }
		{
			m_RectData.StartRecording();
			m_BorderData.StartRecording();
		}

		void EndRecording(SDL_GPUCopyPass* copyPass)
		{
			m_RectData.EndRecording(copyPass);
			m_BorderData.EndRecording(copyPass);
		}

		void AddRectangle(float4 bounds, float4 bgColor, float4 corners)
		{
			m_RectData.Add(
				apollo::rdr::ui::UiRect{
					.Bounds = bounds,
					.BgColor = bgColor,
					.CornerRadius = corners,
				});
			++m_Rectangles.m_NumInstances;
		}
		void AddBorder(float4 bounds, float4 color, float4 width, float4 corners)
		{
			m_BorderData.Add(
				apollo::rdr::ui::Border{
					.Bounds = bounds,
					.Color = color,
					.Width = width,
					.CornerRadius = corners,
				});
			++m_Borders.m_NumInstances;
		}
		void SetScissor(int32 x, int32 y, int32 w, int32 h) noexcept
		{
			m_Scissor = { x, y, w, h };
			m_ScissorSet = true;
		}

		void Flush(
			apollo::rdr::Context& ctx,
			SDL_GPUGraphicsPipeline* rectPipeline,
			SDL_GPUGraphicsPipeline* borderPipeline)
		{
			if (m_ScissorSet)
			{
				ctx.SetScissor(m_Scissor);
			}
			if (m_Rectangles.m_NumInstances)
			{
				ctx.BindGraphicsPipeline(rectPipeline);
				ctx.BindVertexStorageBuffer(m_RectData.GetBuffer());
				ctx.DrawPrimitives(m_Rectangles);
			}
			if (m_Borders.m_NumInstances)
			{
				ctx.BindGraphicsPipeline(borderPipeline);
				ctx.BindVertexStorageBuffer(m_BorderData.GetBuffer());
				ctx.DrawPrimitives(m_Borders);
			}

			m_ScissorSet = false;
			m_Rectangles.m_FistInstance = m_Rectangles.m_NumInstances;
			m_Rectangles.m_NumInstances = 0;
			m_Borders.m_FistInstance = m_Borders.m_NumInstances;
			m_Borders.m_NumInstances = 0;
		}

	private:
		RectangleBatch& m_RectData;
		BorderBatch& m_BorderData;
		DrawCall m_Rectangles;
		DrawCall m_Borders;
		apollo::rdr::ScissorCommand m_Scissor;
		bool m_ScissorSet = false;
	};
} // namespace

#define CHECK_MODULE(var, name)                                                                    \
	DEBUG_CHECK(var)                                                                               \
	{                                                                                              \
		APOLLO_LOG_CRITICAL(                                                                       \
			"Failed to compile built-in " name " module: {:.{}}",                                  \
			static_cast<const char*>(diagnostics->getBufferPointer()),                             \
			diagnostics->getBufferSize());                                                         \
		DEBUG_BREAK();                                                                             \
	}

namespace apollo::rdr::ui {
	Renderer Renderer::s_Instance;

	void Renderer::Init(Context& ctx, rdr::ShaderCompiler& compiler, EPixelFormat fmt)
	{
		if (m_RectPipeline)
		{
			APOLLO_LOG_WARN("Called apollo::rdr::ui::Renderer::Init() twice");
			return;
		}

		Slang::ComPtr<slang::IBlob> diagnostics;
		auto* rectModule = compiler.LoadModuleFromSource(
			&data::shaders::g_UiRectSource,
			"UiRect",
			nullptr,
			diagnostics.writeRef());
		CHECK_MODULE(rectModule, "UiRect");
		auto* borderModule = compiler.LoadModuleFromSource(
			&data::shaders::g_UiBorderSource,
			"UiBorder",
			nullptr,
			diagnostics.writeRef());
		CHECK_MODULE(rectModule, "UiBorder");

		m_RenderContext = &ctx;
		m_RectPipeline = CreatePipeline(
			ctx.GetDevice().GetHandle(),
			"01KN520H22F7ZSD8YPTD6DRP96"_ulid,
			"01KN520HPEJJK0ACQZJGVRXT86"_ulid,
			*rectModule,
			static_cast<SDL_GPUTextureFormat>(fmt),
			compiler);
		m_BorderPipeline = CreatePipeline(
			ctx.GetDevice().GetHandle(),
			"01KN5217NBXDHQFMHKWATE31AV"_ulid,
			"01KN52184ZGNTAF713D5SJXV68"_ulid,
			*borderModule,
			static_cast<SDL_GPUTextureFormat>(fmt),
			compiler);
		m_Rectangles = Batch<UiRect>{ 16 };
		m_Borders = Batch<Border>{ 16 };
	}

	void Renderer::Reset()
	{
		// Forced destroy all GPU buffers
		m_Rectangles = {};
		m_Borders = {};

		if (!m_RectPipeline)
			return;

		SDL_ReleaseGPUGraphicsPipeline(m_RenderContext->GetDevice().GetHandle(), m_RectPipeline);
		SDL_ReleaseGPUGraphicsPipeline(m_RenderContext->GetDevice().GetHandle(), m_BorderPipeline);
		m_RenderContext = nullptr;
		m_RectPipeline = nullptr;
		m_BorderPipeline = nullptr;
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

		DrawData data{ m_Rectangles, m_Borders };
		int32 zIndex = INT32_MIN;
		m_RenderContext->PushVertexShaderConstants(m_ProjMatrix);

		for (const auto& cmd : commands)
		{
			if (cmd.zIndex != zIndex)
			{
				data.Flush(*m_RenderContext, m_RectPipeline, m_BorderPipeline);
				zIndex = cmd.zIndex;
			}

			switch (cmd.commandType)
			{
			case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
			{
				const float maxRadius = Min(cmd.boundingBox.width, cmd.boundingBox.height) / 2.0f;
				data.AddRectangle(
					float4{
						cmd.boundingBox.x,
						cmd.boundingBox.y,
						cmd.boundingBox.width,
						cmd.boundingBox.height,
					},
					float4{
						cmd.renderData.rectangle.backgroundColor.r,
						cmd.renderData.rectangle.backgroundColor.g,
						cmd.renderData.rectangle.backgroundColor.b,
						cmd.renderData.rectangle.backgroundColor.a,
					},
					float4{
						Min(maxRadius, cmd.renderData.rectangle.cornerRadius.topLeft),
						Min(maxRadius, cmd.renderData.rectangle.cornerRadius.topRight),
						Min(maxRadius, cmd.renderData.rectangle.cornerRadius.bottomLeft),
						Min(maxRadius, cmd.renderData.rectangle.cornerRadius.bottomRight),
					});
			}
			break;

			case CLAY_RENDER_COMMAND_TYPE_BORDER:
			{
				const float maxRadius = Min(cmd.boundingBox.width, cmd.boundingBox.height) / 2.0f;
				data.AddBorder(
					float4{
						cmd.boundingBox.x,
						cmd.boundingBox.y,
						cmd.boundingBox.width,
						cmd.boundingBox.height,
					},
					float4{
						cmd.renderData.border.color.r,
						cmd.renderData.border.color.g,
						cmd.renderData.border.color.b,
						cmd.renderData.border.color.a,
					},
					float4{
						cmd.renderData.border.width.left,
						cmd.renderData.border.width.right,
						cmd.renderData.border.width.top,
						cmd.renderData.border.width.bottom,
					},
					float4{
						Min(maxRadius, cmd.renderData.border.cornerRadius.topLeft),
						Min(maxRadius, cmd.renderData.border.cornerRadius.topRight),
						Min(maxRadius, cmd.renderData.border.cornerRadius.bottomLeft),
						Min(maxRadius, cmd.renderData.border.cornerRadius.bottomRight),
					});
			}
			break;

			case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
				data.Flush(*m_RenderContext, m_RectPipeline, m_BorderPipeline);
				data.SetScissor(
					cmd.boundingBox.x,
					cmd.boundingBox.y,
					cmd.boundingBox.width,
					cmd.boundingBox.height);
				break;
			case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
				data.Flush(*m_RenderContext, m_RectPipeline, m_BorderPipeline);
				data.SetScissor(0, 0, m_TargetSize.x, m_TargetSize.y);
				break;
			default: break;
			}
		}

		data.EndRecording(copyPass);
		SDL_EndGPUCopyPass(copyPass);

		data.Flush(*m_RenderContext, m_RectPipeline, m_BorderPipeline);
	}
} // namespace apollo::rdr::ui