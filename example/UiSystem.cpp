#include "UiSystem.hpp"
#include <SDL3/SDL_gpu.h>
#include <ui/Context.hpp>
#include <ui/Element.hpp>
#include <ui/Renderer.hpp>

namespace {
	[[maybe_unused]] constexpr Clay_Color g_Red = (Clay_Color){ 0.66f, 0.26, 0.11f, 1.0 };
	[[maybe_unused]] constexpr Clay_Color g_LightGray = (Clay_Color){ 0.66f, 0.66f, 0.66f, 1.f };

	constexpr Clay_ElementDeclaration g_SidebarConfig{
		.layout =
			Clay_LayoutConfig{
				.sizing = { .width = CLAY_SIZING_PERCENT(0.2f), .height = CLAY_SIZING_GROW() },
				.padding = { 10, 10, 10, 10 },
				.childGap = 10,
				.layoutDirection = CLAY_TOP_TO_BOTTOM,
			},
		// .backgroundColor = g_Red,
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
	UiSystem::~UiSystem()
	{
		APOLLO_LOG_INFO("Cleaning UI system");
		m_UiRenderer.Reset();
		m_UiContext.Reset();
	}

	void UiSystem::PostInit()
	{
		m_UiRenderer.Init(m_RenderContext, m_Viewport.m_ColorTargetFormat);
		m_UiContext.Init(m_UiRenderer, 0, 0);
	}

	void UiSystem::Update(entt::registry&, const GameTime&)
	{
		const float2 winSize{
			m_Viewport.m_Rectangle.x1 - m_Viewport.m_Rectangle.x0,
			m_Viewport.m_Rectangle.y1 - m_Viewport.m_Rectangle.y0,
		};
		m_UiRenderer.SetTargetSize(m_Viewport.m_TargetSize);
		m_UiContext.SetSize(winSize.x, winSize.y);
		m_UiContext.BeginLayout();

		BuildLayout();

		m_RenderContext.BeginRenderPass(m_RenderPass);
		m_UiContext.EndLayout();
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