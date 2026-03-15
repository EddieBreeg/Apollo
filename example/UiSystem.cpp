#include "UiSystem.hpp"
#include <SDL3/SDL_gpu.h>
#include <ui/Context.hpp>
#include <ui/Element.hpp>

namespace {
	constexpr Clay_Color g_Red = (Clay_Color){ 168, 66, 28, 255 };
	constexpr Clay_Color g_LightGray = (Clay_Color){
		168,
		168,
		168,
		255,
	};

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
	};

	apollo::demo::UiRect CreateElement(const Clay_RenderCommand& cmd)
	{
		apollo::demo::UiRect rect{
			.Rectangle =
				float4{
					cmd.boundingBox.x,
					cmd.boundingBox.y,
					cmd.boundingBox.width,
					cmd.boundingBox.height,
				},
		};
		switch (cmd.commandType)
		{
		case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
			rect.BgColor = float4{
				cmd.renderData.rectangle.backgroundColor.r,
				cmd.renderData.rectangle.backgroundColor.g,
				cmd.renderData.rectangle.backgroundColor.b,
				cmd.renderData.rectangle.backgroundColor.a,
			} / 255.0f;
			rect.CornerRadius = float4{
				cmd.renderData.rectangle.cornerRadius.topLeft,
				cmd.renderData.rectangle.cornerRadius.topRight,
				cmd.renderData.rectangle.cornerRadius.bottomLeft,
				cmd.renderData.rectangle.cornerRadius.bottomRight,
			};
			break;
		case CLAY_RENDER_COMMAND_TYPE_BORDER:
			rect.BorderColor = float4{
				cmd.renderData.border.color.r,
				cmd.renderData.border.color.g,
				cmd.renderData.border.color.b,
				cmd.renderData.border.color.a,
			} / 255.0f;
			rect.CornerRadius = float4{
				cmd.renderData.border.cornerRadius.topLeft,
				cmd.renderData.border.cornerRadius.topRight,
				cmd.renderData.border.cornerRadius.bottomLeft,
				cmd.renderData.border.cornerRadius.bottomRight,
			};
			rect.BorderThickness = float4{
				cmd.renderData.border.width.left,
				cmd.renderData.border.width.right,
				cmd.renderData.border.width.top,
				cmd.renderData.border.width.bottom,
			};
		default: break;
		}
		return rect;
	}

	void DrawElement(const apollo::demo::UiRect& elem, apollo::rdr::Context& ctx)
	{
		auto* cmdBuffer = ctx.GetMainCommandBuffer();
		SDL_PushGPUVertexUniformData(cmdBuffer, 1, &elem.Rectangle, sizeof(elem.Rectangle));
		SDL_PushGPUFragmentUniformData(cmdBuffer, 0, &elem, sizeof(elem));
		SDL_DrawGPUPrimitives(ctx.GetCurrentRenderPass()->GetHandle(), 4, 1, 0, 0);
	}
} // namespace

namespace apollo::demo {
	void UiSystem::PostInit()
	{
		m_UiContext.Init(0, 0);

		auto* manager = IAssetManager::GetInstance();
		m_QuadMat = manager->GetAsset<rdr::Material>("01KKPKX9AA2VHB6GRRF4T967DP"_ulid);
	}

	void UiSystem::Update(entt::registry&, const GameTime&)
	{
		m_Elements.clear();

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

		m_RenderContext.PushVertexShaderConstants(m_ProjMatrix);
		m_RenderContext.BeginRenderPass(m_RenderPass);
		m_RenderContext.BindGraphicsPipeline(
			static_cast<SDL_GPUGraphicsPipeline*>(m_QuadMat->GetHandle()));

		m_UiContext.BeginLayout();

		BuildLayout();

		auto renderCommands = m_UiContext.EndLayout();
		std::span commandArray{
			renderCommands.internalArray,
			static_cast<size_t>(renderCommands.length),
		};
		m_Elements.reserve(commandArray.size());

		for (const auto& cmd : commandArray)
		{
			m_Elements.emplace_back(CreateElement(cmd));
		}

		m_RenderContext.AddCustomCommand(
			[this](rdr::Context& ctx)
			{
				for (const auto& elem : m_Elements)
					DrawElement(elem, ctx);
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