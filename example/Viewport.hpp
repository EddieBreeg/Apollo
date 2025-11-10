#pragma once

#include "glm/ext/vector_uint2.hpp"
#include "rendering/Pixel.hpp"
#include <PCH.hpp>
#include <SDL3/SDL_gpu.h>
#include <imgui.h>
#include <rendering/Texture.hpp>

namespace apollo::demo {
	struct Viewport
	{
		const char* m_Title = "Scene View";
		rdr::EPixelFormat m_ColorTargetFormat = rdr::EPixelFormat::RGBA8_UNorm_SRGB;
		rdr::Texture2D m_ColorTarget;
		rdr::Texture2D m_DepthStencilTarget;
		RectF m_Rectangle;
		float2 m_TargetSize;
		bool m_ShowDepth = false;

		void Update()
		{
			ImGui::Begin(m_Title);

			const auto* vp = ImGui::GetWindowViewport();
			const glm::uvec2 vpSize{ uint32(vp->WorkSize.x), uint32(vp->WorkSize.y) };

			if (vpSize.x != m_TargetSize.x || vpSize.y != m_TargetSize.y)
			{
				m_TargetSize = { vpSize.x, vpSize.y };
				CreateTargets(vpSize.x, vpSize.y);
			}

			const ImVec2 winPos = ImGui::GetWindowPos();
			ImVec2 topLeft = ImGui::GetWindowContentRegionMin();
			ImVec2 bottomRight = ImGui::GetWindowContentRegionMax();

			const ImVec2 winSize{
				bottomRight.x - topLeft.x,
				bottomRight.y - topLeft.y,
			};

			topLeft.x += winPos.x;
			topLeft.y += winPos.y;

			bottomRight.x += winPos.x;
			bottomRight.y += winPos.y;

			m_Rectangle = {
				.x1 = winSize.x,
				.y1 = winSize.y,
			};
			const ImTextureID texId = (ImTextureID)(m_ShowDepth ? m_DepthStencilTarget.GetHandle()
																: m_ColorTarget.GetHandle());

			ImGui::GetWindowDrawList()->AddImage(
				texId,
				topLeft,
				bottomRight,
				{ 0, 0 },
				{ winSize.x / m_TargetSize.x, winSize.y / m_TargetSize.y });

			ImGui::End();
		}

	private:
		void CreateTargets(uint32 w, uint32 h)
		{
			m_ColorTarget = rdr::Texture2D{
				rdr::TextureSettings{
					.m_Width = w,
					.m_Height = h,
					.m_Format = m_ColorTargetFormat,
					.m_Usage = rdr::ETextureUsageFlags::ColorTarget |
							   rdr::ETextureUsageFlags::Sampled,
				},
			};
			m_DepthStencilTarget = rdr::Texture2D{
				rdr::TextureSettings{
					.m_Width = w,
					.m_Height = h,
					.m_Format = rdr::EPixelFormat::Depth32,
					.m_Usage = rdr::ETextureUsageFlags::DepthStencilTarget |
							   rdr::ETextureUsageFlags::Sampled,
				},
			};
		}
	};
} // namespace apollo::demo