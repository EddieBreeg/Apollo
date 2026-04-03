#pragma once

#include "DemoPCH.hpp"

namespace apollo::rdr {
	class ShaderCompiler;
	enum class EPixelFormat : int8;
} // namespace apollo::rdr

namespace apollo::rdr::ui {
	class Renderer;
}

namespace apollo::ui {
	class Context;
}

struct Clay_RenderCommand;

namespace apollo::demo {
	class UiSystem
	{
	public:
		UiSystem(
			rdr::Context& renderContext,
			ui::Context& uiContext,
			rdr::ui::Renderer& uiRenderer,
			rdr::ShaderCompiler& compiler,
			const Viewport& vp)
			: m_RenderContext(renderContext)
			, m_UiContext(uiContext)
			, m_UiRenderer(uiRenderer)
			, m_Compiler(compiler)
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

		~UiSystem() = default;

	private:
		void BuildLayout();

		rdr::Context& m_RenderContext;
		ui::Context& m_UiContext;
		rdr::ui::Renderer& m_UiRenderer;
		rdr::ShaderCompiler& m_Compiler;
		rdr::RenderPass m_RenderPass;
		const Viewport& m_Viewport;
	};
} // namespace apollo::demo