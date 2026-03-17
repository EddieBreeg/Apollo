#include "UiSystem.hpp"
#include <SDL3/SDL_gpu.h>
#include <ui/Context.hpp>
#include <ui/Element.hpp>

namespace {
	constexpr Clay_Color g_Red = (Clay_Color){ 0.66f, 0.26, 0.11f, 1.0 };
	constexpr Clay_Color g_LightGray = (Clay_Color){ 0.66f, 0.66f, 0.66f, 1.f };

	constexpr Clay_ElementDeclaration g_SidebarConfig{
		.layout =
			Clay_LayoutConfig{
				.sizing = { .width = CLAY_SIZING_PERCENT(0.2f), .height = CLAY_SIZING_GROW() },
				.padding = { 10, 10, 10, 10 },
				.childGap = 10,
				.layoutDirection = CLAY_TOP_TO_BOTTOM,
			},
		.backgroundColor = g_Red,
	};

	constexpr Clay_ElementDeclaration g_SidebarChildConfig{
		.layout =
			Clay_LayoutConfig{
				.sizing =
					Clay_Sizing{
						CLAY_SIZING_GROW(0),
						CLAY_SIZING_FIXED(50),
					},
			},
		.backgroundColor = g_LightGray,
		.cornerRadius =
			Clay_CornerRadius{
				.topLeft = 25,
				.topRight = 25,
				.bottomLeft = 25,
				.bottomRight = 25,
			},
	};
} // namespace

namespace apollo::demo {
	UiLayer::UiLayer(AssetRef<rdr::Material> mat)
		: m_Rect(mat, 16)
		, m_Borders(std::move(mat), 16)
	{}

	void UiLayer::StartRecording()
	{
		m_Rect.Clear();
		m_Rect.StartRecording();
		m_Borders.Clear();
		m_Borders.StartRecording();
	}
	void UiLayer::EndRecording(SDL_GPUCopyPass* copyPass)
	{
		m_Rect.EndRecording(copyPass);
		m_Borders.EndRecording(copyPass);
	}

	void UiLayer::AddCommand(const Clay_RenderCommand& cmd)
	{
		switch (cmd.commandType)
		{
		case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
			m_Rect.Add(
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
							cmd.renderData.rectangle.cornerRadius.topLeft,
							cmd.renderData.rectangle.cornerRadius.topRight,
							cmd.renderData.rectangle.cornerRadius.bottomLeft,
							cmd.renderData.rectangle.cornerRadius.bottomRight,
						},
				});
			break;
		case CLAY_RENDER_COMMAND_TYPE_BORDER:
			m_Rect.Add(
				UiRect{
					.BorderColor =
						float4{
							cmd.renderData.border.color.r,
							cmd.renderData.border.color.g,
							cmd.renderData.border.color.b,
							cmd.renderData.border.color.a,
						},
					.BorderThickness =
						float4{
							cmd.renderData.border.width.left,
							cmd.renderData.border.width.right,
							cmd.renderData.border.width.top,
							cmd.renderData.border.width.bottom,
						},
					.CornerRadius =
						float4{
							cmd.renderData.border.cornerRadius.topLeft,
							cmd.renderData.border.cornerRadius.topRight,
							cmd.renderData.border.cornerRadius.bottomLeft,
							cmd.renderData.border.cornerRadius.bottomRight,
						},
				});
			break;
		default: break;
		}
	}

	void UiLayer::Draw(rdr::Context& ctx)
	{
		auto* renderPass = ctx.GetCurrentRenderPass();
		if (!renderPass)
			return;

		auto* buf = m_Rect.GetBuffer().GetHandle();
		if (m_Rect.GetCount())
		{
			SDL_BindGPUGraphicsPipeline(renderPass->GetHandle(), m_Rect.GetPipeline());
			SDL_BindGPUVertexStorageBuffers(renderPass->GetHandle(), 0, &buf, 1);
			SDL_DrawGPUPrimitives(renderPass->GetHandle(), 4, m_Rect.GetCount(), 0, 0);
		}

		if (m_Borders.GetCount())
		{
			SDL_BindGPUGraphicsPipeline(renderPass->GetHandle(), m_Borders.GetPipeline());
			buf = m_Borders.GetBuffer().GetHandle();
			SDL_BindGPUVertexStorageBuffers(renderPass->GetHandle(), 0, &buf, 1);
			SDL_DrawGPUPrimitives(renderPass->GetHandle(), 4, m_Borders.GetCount(), 0, 0);
		}
	}

	void UiSystem::PostInit()
	{
		m_UiContext.Init(0, 0);
		m_UiLayer = UiLayer{ m_QuadMat };

		auto* manager = IAssetManager::GetInstance();
		m_QuadMat = manager->GetAsset<rdr::Material>("01KKPKX9AA2VHB6GRRF4T967DP"_ulid);
	}

	void UiSystem::Update(entt::registry&, const GameTime&)
	{
		const float2 winSize{
			m_Viewport.m_Rectangle.x1 - m_Viewport.m_Rectangle.x0,
			m_Viewport.m_Rectangle.y1 - m_Viewport.m_Rectangle.y0,
		};
		if (m_UiContext.SetSize(winSize.x, winSize.y))
		{
			m_ProjMatrix = glm::orthoRH(
				0.0f,
				m_Viewport.m_TargetSize.x,
				m_Viewport.m_TargetSize.y,
				0.0f,
				0.0f,
				1.0f);
		}

		if (!m_QuadMat->IsLoaded())
			return;

		m_UiContext.BeginLayout();

		BuildLayout();

		auto renderCommands = m_UiContext.EndLayout();
		std::span commandArray{
			renderCommands.internalArray,
			static_cast<size_t>(renderCommands.length),
		};
		m_UiLayer.StartRecording();
		for (const auto& cmd : commandArray)
		{
			m_UiLayer.AddCommand(cmd);
		}
		
		auto* cmdBuffer = m_RenderContext.GetMainCommandBuffer();
		auto* copyPass = SDL_BeginGPUCopyPass(cmdBuffer);
		m_UiLayer.EndRecording(copyPass);
		SDL_EndGPUCopyPass(copyPass);

		m_RenderContext.PushVertexShaderConstants(m_ProjMatrix);
		m_RenderContext.BeginRenderPass(m_RenderPass);
		m_RenderContext.BindGraphicsPipeline(
			static_cast<SDL_GPUGraphicsPipeline*>(m_QuadMat->GetHandle()));

		m_RenderContext.AddCustomCommand(
			[&](rdr::Context& ctx)
			{
				m_UiLayer.Draw(ctx);
			});
	}

	void UiSystem::BuildLayout()
	{
		using namespace ui::ui_literals;

		ui::Element sidebar("sidebar"_eid, g_SidebarConfig);
		for (int i = 0; i < 3; ++i)
		{
			const Clay_ElementId id = ui::CreateId("child"_cstr, i);
			sidebar.AddChild<ui::Element>(id, g_SidebarChildConfig);
		}
	}
} // namespace apollo::demo