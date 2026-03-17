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

		[[nodiscard]] bool operator!=(const UiRect& other) const noexcept
		{
			return Rectangle != other.Rectangle || BgColor != other.BgColor ||
				   BorderColor != other.BorderColor || BorderThickness != other.BorderThickness ||
				   CornerRadius != other.CornerRadius;
		}
	};

	class UiLayer
	{
	public:
		UiLayer() = default;
		UiLayer(AssetRef<rdr::Material> material);

		UiLayer(UiLayer&& other) = default;
		UiLayer(const UiLayer&) = delete;
		UiLayer& operator=(UiLayer&& other) = default;
		UiLayer& operator=(const UiLayer&) = delete;

		void StartRecording();
		void EndRecording(SDL_GPUCopyPass* copyPass);

		void AddCommand(const Clay_RenderCommand& cmd);
		void Draw(rdr::Context& ctx);

	private:
		rdr::Batch<UiRect> m_Rect;
		rdr::Batch<UiRect> m_Borders;
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
		UiLayer m_UiLayer;

		AssetRef<rdr::Material> m_QuadMat;
		glm::mat4x4 m_ProjMatrix;
	};
} // namespace apollo::demo