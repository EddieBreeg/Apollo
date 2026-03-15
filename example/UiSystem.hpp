#pragma once

#include "DemoPCH.hpp"

namespace apollo::ui {
	class Context;
}

struct Clay_RenderCommand;

namespace apollo::demo {
	struct UiRect
	{
		GPU_ALIGN(float4) Rectangle = {};
		GPU_ALIGN(float4) BgColor = {};
		GPU_ALIGN(float4) BorderColor = {};
		GPU_ALIGN(float4) BorderThickness = {};
		GPU_ALIGN(float4) CornerRadius = {};
	};

	class UiSystem
	{
	public:
		UiSystem(rdr::Context& renderContext, ui::Context& uiContext, const Viewport& vp)
			: m_RenderContext(renderContext)
			, m_UiContext(uiContext)
			, m_RenderPass(
				  rdr::RenderPassSettings{
					  .m_NumColorTargets = 1,
					  .m_ColorTargets{
						  rdr::ColorTargetSettings{
							  .m_Texture = &vp.m_ColorTarget,
							  .m_LoadOp = rdr::ELoadOp::Load,
						  },
					  },
				  })
			, m_Viewport(vp)
		{}

		void PostInit();
		void Update(entt::registry&, const GameTime&);

	private:
		void BuildLayout();

		rdr::Context& m_RenderContext;
		ui::Context& m_UiContext;
		rdr::RenderPass m_RenderPass;
		const Viewport& m_Viewport;
		std::vector<UiRect> m_Elements;

		AssetRef<rdr::Material> m_QuadMat;
		glm::mat4x4 m_ProjMatrix;
	};
} // namespace apollo::demo