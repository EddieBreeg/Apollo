#include "CameraSystem.hpp"
#include "Inspector.hpp"
#include "VisualSystem.hpp"
#include <editor/asset/Manager.hpp>
#include <ui/Context.hpp>
#include <ui/Renderer.hpp>

namespace ImGui {
	void ShowDemoWindow(bool* p_open);
}

namespace apollo::demo {
	glm::mat4x4 GetProjMatrix(const Viewport& vp)
	{
		return glm::perspectiveFovRH(
			0.5f * std::numbers::pi_v<float>,
			vp.m_Rectangle.GetWidth(),
			vp.m_Rectangle.GetHeight(),
			0.01f,
			100.f);
	}

	template <class UInt, uint8 P>
	[[nodiscard]] UInt ToFixedPoint(float x)
	{
		const UInt pow2 = UInt(1) << P;
		constexpr UInt mask = pow2 - 1;
		const float rounding = x < 0 ? -0.5f : 0.5f;

		return static_cast<UInt>((x * pow2 + rounding)) & mask;
	}

	uint64 ComputeKey(const rdr::MaterialInstance& material, float distance)
	{
		const auto matKey = material.GetKey();
		if (matKey.WritesToDepthBuffer())
		{
			return ((uint64)matKey << 24) | ToFixedPoint<uint64, 24>(distance);
		}
		else
		{
			return BIT(54) | (ToFixedPoint<uint64, 24>(-distance) << 30) | (uint64)matKey;
		}
	}

	VisualElement::VisualElement(
		const MeshComponent& meshComp,
		const TransformComponent& transform,
		float3 camPos,
		float3 viewVec)
		: m_Transform(&transform)
		, m_MeshComp(&meshComp)
		, m_Type(EType::Mesh)
	{
		const float distance = glm::dot(transform.m_Position - camPos, viewVec) / 100.f;
		m_Key = ComputeKey(*meshComp.m_Material, distance);
	}
	VisualElement::VisualElement(
		const GridComponent& gridComponent,
		const TransformComponent& transform,
		float3 camPos,
		float3 viewVec)
		: m_Transform(&transform)
		, m_Grid(&gridComponent)
		, m_Type(EType::Grid)
	{
		const float distance = glm::dot(transform.m_Position - camPos, viewVec) / 100.f;
		m_Key = ComputeKey(*gridComponent.m_Mat, distance);
	}

	void VisualElement::Draw(SDL_GPUCommandBuffer* cmdBuffer, SDL_GPURenderPass* pass) const
	{
		switch (m_Type)
		{
		case EType::Mesh: DrawMesh(cmdBuffer, pass); break;
		case EType::Grid:
			DrawGrid(cmdBuffer, pass);
			break;

		[[unlikely]] default:
			APOLLO_LOG_CRITICAL("Invalid visual element type: {}", ToUnderlying(m_Type));
			DEBUG_BREAK();
			break;
		}
	}

	void VisualElement::DrawMesh(SDL_GPUCommandBuffer* cmdBuffer, SDL_GPURenderPass* pass) const
	{
		m_MeshComp->m_Material->Bind(pass);
		m_MeshComp->m_Material->PushFragmentConstants(cmdBuffer);
		const auto modelMat = ComputeTransformMatrix(
			m_Transform->m_Position,
			m_Transform->m_Scale,
			m_Transform->m_Rotation);
		SDL_PushGPUVertexUniformData(cmdBuffer, 1u, &modelMat, sizeof(modelMat));
		const auto& vBuffer = m_MeshComp->m_Mesh->GetVertexBuffer();
		SDL_GPUBufferBinding binding{ .buffer = vBuffer.GetHandle() };
		SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
		const auto& iBuffer = m_MeshComp->m_Mesh->GetIndexBuffer();
		if (iBuffer)
		{
			binding.buffer = iBuffer.GetHandle();
			SDL_BindGPUIndexBuffer(pass, &binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
			SDL_DrawGPUIndexedPrimitives(pass, m_MeshComp->m_Mesh->GetNumIndices(), 1, 0, 0, 0);
		}
		else
		{
			SDL_DrawGPUPrimitives(pass, m_MeshComp->m_Mesh->GetNumVertices(), 1, 0, 0);
		}
	}
	void VisualElement::DrawGrid(SDL_GPUCommandBuffer* cmdBuffer, SDL_GPURenderPass* pass) const
	{
		const struct VertexData
		{
			glm::mat4x4 Transform;
			uint32 Width;
		} vertexData{
			ComputeTransformMatrix(
				m_Transform->m_Position,
				m_Transform->m_Scale,
				m_Transform->m_Rotation),
			m_Grid->m_GridWidth,
		};
		SDL_PushGPUVertexUniformData(cmdBuffer, 1u, &vertexData, sizeof(vertexData));
		m_Grid->m_Mat->Bind(pass);
		const uint32 numInstances = vertexData.Width * vertexData.Width;
		SDL_DrawGPUPrimitives(pass, 4, numInstances, 0, 0);
	}

	VisualSystem::VisualSystem(
		apollo::Window& window,
		apollo::rdr::Context& renderer,
		CameraSystem& camSystem)
		: m_Window(window)
		, m_RenderContext(renderer)
		, m_CamSystem(camSystem)
		, m_RenderPass(
			  rdr::RenderPassSettings{
				  .m_NumColorTargets = 1,
				  .m_ColorTargets = { rdr::ColorTargetSettings{
					  .m_Texture = &m_TargetViewport.m_ColorTarget,
					  .m_ClearColor = { .4f, .4f, .4f, 1.0f },
				  } },
				  .m_DepthStencilTarget =
					  rdr::DepthStencilTargetSettings{
						  .m_Texture = &m_TargetViewport.m_DepthStencilTarget,
					  },
			  })
	{
		m_TargetViewport.m_ColorTargetFormat = rdr::EPixelFormat::RGBA8_UNorm;
	}
	void VisualSystem::DisplayUi(entt::registry& world)
	{
		m_TargetViewport.Update();
		m_Inspector.Update(world);

		ImGui::Begin("Settings & Info");
		ImGui::SliderFloat("Mouse Speed", &m_CamSystem.m_MouseSpeed, .0f, 10.0f);
		ImGui::SliderFloat("Move Speed", &m_CamSystem.m_CameraSpeed, .0f, 10.0f);
		ImGui::Text("Press F1 to unlock/relock camera");

		ImGui::Checkbox("Show Inspector", &m_Inspector.m_ShowInspector);
		ImGui::Checkbox("Show Depth/Stencil Content", &m_TargetViewport.m_ShowDepth);

		ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);
		ImGui::End();
	}

	void VisualSystem::EmitGPUCommands(const entt::registry& world)
	{
		const auto vpMatrix = GetProjMatrix(m_TargetViewport) * m_CamSystem.GetViewMatrix();
		m_RenderContext.PushVertexShaderConstants(vpMatrix, 0);
		m_RenderContext.BeginRenderPass(m_RenderPass);
		m_RenderContext.SetViewport(m_TargetViewport.m_Rectangle);

		const auto meshView = world.view<const MeshComponent, const TransformComponent>();
		const auto gridView = world.view<const GridComponent, const TransformComponent>();

		m_VisualElements.clear();
		m_VisualElements.reserve(meshView.size_hint() + gridView.size_hint());
		const Camera& cam = m_CamSystem.GetCamera();

		for (const auto entt : meshView)
		{
			const auto& mesh = meshView.get<const MeshComponent>(entt);
			if (!mesh.m_Mesh || !mesh.m_Material || !mesh.m_Mesh->IsLoaded() ||
				!mesh.m_Material->IsLoaded())
				continue;

			const auto& transform = meshView.get<const TransformComponent>(entt);
			m_VisualElements.emplace_back(mesh, transform, cam.GetTranslate(), cam.GetForward());
		}
		for (const auto entt : gridView)
		{
			const auto& grid = gridView.get<const GridComponent>(entt);
			if (!grid.m_Mat || !grid.m_Mat->IsLoaded() || !grid.m_GridWidth)
				continue;

			const auto& transform = meshView.get<const TransformComponent>(entt);
			m_VisualElements.emplace_back(grid, transform, cam.GetTranslate(), cam.GetForward());
		}
		std::sort(m_VisualElements.begin(), m_VisualElements.end());

		m_RenderContext.AddCustomCommand(
			[this](rdr::Context& ctx)
			{
				auto* pass = ctx.GetCurrentRenderPass()->GetHandle();
				auto* const cmdBuffer = ctx.GetMainCommandBuffer();

				for (const auto& e : m_VisualElements)
				{
					e.Draw(cmdBuffer, pass);
				}
			});
	}

	void VisualSystem::Update(entt::registry& world, const apollo::GameTime&)
	{
		if (!m_Window) [[unlikely]]
			return;

		if (const auto view = world.view<const SceneLoadFinishedEventComponent>(); view.size())
		{
			const entt::entity sceneEntity = *view.begin();
			m_Inspector.m_Scene = world.get<const SceneComponent>(sceneEntity).m_Scene;
		}

		auto* swapchainTexture = m_RenderContext.GetSwapchainTexture();

		if (!swapchainTexture)
			return;

		DisplayUi(world);
		EmitGPUCommands(world);
	}
} // namespace apollo::demo