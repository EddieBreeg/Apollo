#pragma once

#include "DemoPCH.hpp"
#include <core/Transform.hpp>

namespace apollo::demo {
	struct GridComponent
	{
		AssetRef<rdr::MaterialInstance> m_Mat;
		uint32 m_GridWidth = 3;

		static constexpr ecs::ComponentReflection<&GridComponent::m_Mat, &GridComponent::m_GridWidth>
			Reflection{
				"grid",
				{ "material", "width" },
			};
	};
	struct GridDrawSystem
	{
		GridDrawSystem(rdr::Context& ctx, const Viewport& targetViewport, const Camera& cam)
			: m_RenderContext(ctx)
			, m_TargetViewport(targetViewport)
			, m_RenderPass(
				  rdr::RenderPassSettings{
					  .m_NumColorTargets = 1,
					  .m_ColorTargets = { rdr::ColorTargetSettings{
						  .m_Texture = &m_TargetViewport.m_ColorTarget,
						  .m_LoadOp = rdr::ELoadOp::Load,
					  }, },
					  .m_DepthStencilTarget =
						  rdr::DepthStencilTargetSettings{
							  .m_Texture = &m_TargetViewport.m_DepthStencilTarget,
						  },
				  })
			, m_Camera(cam)
		{}

		void Update(entt::registry& world, const GameTime&) noexcept
		{
			const rdr::MaterialInstance* mat = nullptr;
			{
				const auto view = world.view<const GridComponent, const TransformComponent>();

				struct VertexData
				{
					glm::mat4x4 Transform;
					uint32 Width;
				};

				const auto vpMat = glm::perspectiveFovRH(
									   0.5f * std::numbers::pi_v<float>,
									   m_TargetViewport.m_TargetSize.x,
									   m_TargetViewport.m_TargetSize.y,
									   0.01f,
									   100.f) *
								   m_Camera.GetLookAt();
				m_RenderContext.BeginRenderPass(m_RenderPass);
				m_RenderContext.PushVertexShaderConstants(vpMat, 0);

				for (const auto entity : view)
				{
					const GridComponent& comp = view.get<const GridComponent>(entity);
					if (!comp.m_Mat->IsLoaded())
						continue;

					const auto& transform = view.get<const TransformComponent>(entity);
					const VertexData data{
						.Transform = ComputeTransformMatrix(
							transform.m_Position,
							transform.m_Scale,
							transform.m_Rotation),
						.Width = comp.m_GridWidth,
					};
					m_RenderContext.PushVertexShaderConstants(data, 1);

					if (comp.m_Mat.Get() != mat)
					{
						mat = comp.m_Mat.Get();
						m_RenderContext.BindMaterialInstance(*mat);
					}
					m_RenderContext.DrawPrimitives(
						rdr::DrawCall{
							.m_NumVertices = 4,
							.m_NumInstances = comp.m_GridWidth * comp.m_GridWidth,
						});
				}
			}
		}

		rdr::Context& m_RenderContext;
		const Viewport& m_TargetViewport;
		rdr::RenderPass m_RenderPass;
		const Camera& m_Camera;
	};
} // namespace apollo::demo