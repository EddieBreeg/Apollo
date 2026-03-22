#include "DemoPCH.hpp"

#include "CameraSystem.hpp"
#include "GridSystem.hpp"
#include "Inspector.hpp"
#include "UiSystem.hpp"
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

	struct VisualElement
	{
		VisualElement(
			const MeshComponent& meshComp,
			const TransformComponent& transform,
			float3 camPos,
			float3 viewVec)
			: m_Transform(&transform)
			, m_MeshComp(&meshComp)
		{
			const rdr::MaterialInstanceKey matKey = meshComp.m_Material->GetKey();
			const float distance = glm::dot(transform.m_Position - camPos, viewVec) / 100.f;
			if (matKey.WritesToDepthBuffer())
			{
				m_Key = ((uint64)matKey << 24) | ToFixedPoint<uint64, 24>(distance);
			}
			else
			{
				m_Key = BIT(54) | (ToFixedPoint<uint64, 24>(-distance) << 30) | (uint64)matKey;
			}
		}

		const TransformComponent* m_Transform;
		const MeshComponent* m_MeshComp;
		uint64 m_Key;
	};

	[[nodiscard]] bool operator<(const VisualElement& lhs, const VisualElement& rhs) noexcept
	{
		return lhs.m_Key < rhs.m_Key;
	}

	struct TestSystem
	{
		apollo::Window& m_Window;
		apollo::rdr::Context& m_RenderContext;
		CameraSystem& m_CamSystem;
		Viewport m_TargetViewport;
		Inspector m_Inspector;
		rdr::RenderPass m_RenderPass;

		std::vector<VisualElement> m_VisualElements;

		TestSystem(apollo::Window& window, apollo::rdr::Context& renderer, CameraSystem& camSystem)
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

		~TestSystem() {}

		void DisplayUi(entt::registry& world)
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

		void EmitGPUCommands(const entt::registry& world)
		{
			const auto vpMatrix = GetProjMatrix(m_TargetViewport) * m_CamSystem.GetViewMatrix();
			m_RenderContext.PushVertexShaderConstants(vpMatrix, 0);
			m_RenderContext.BeginRenderPass(m_RenderPass);
			m_RenderContext.SetViewport(m_TargetViewport.m_Rectangle);
			const auto view = world.view<const MeshComponent, const TransformComponent>();

			m_VisualElements.clear();
			m_VisualElements.reserve(view.size_hint());
			const Camera& cam = m_CamSystem.GetCamera();

			for (const auto entt : view)
			{
				const auto& mesh = view.get<const MeshComponent>(entt);
				if (!mesh.m_Mesh || !mesh.m_Material || !mesh.m_Mesh->IsLoaded() ||
					!mesh.m_Material->IsLoaded())
					continue;

				const auto& transform = view.get<const TransformComponent>(entt);
				m_VisualElements.emplace_back(mesh, transform, cam.GetTranslate(), cam.GetForward());
			}
			std::sort(m_VisualElements.begin(), m_VisualElements.end());

			m_RenderContext.AddCustomCommand(
				[this](rdr::Context& ctx)
				{
					auto* pass = ctx.GetCurrentRenderPass()->GetHandle();
					auto* const cmdBuffer = ctx.GetMainCommandBuffer();

					for (const auto& e : m_VisualElements)
					{
						e.m_MeshComp->m_Material->Bind(pass);
						e.m_MeshComp->m_Material->PushFragmentConstants(cmdBuffer);
						const auto modelMat = ComputeTransformMatrix(
							e.m_Transform->m_Position,
							e.m_Transform->m_Scale,
							e.m_Transform->m_Rotation);
						SDL_PushGPUVertexUniformData(cmdBuffer, 1u, &modelMat, sizeof(modelMat));
						const auto& vBuffer = e.m_MeshComp->m_Mesh->GetVertexBuffer();
						SDL_GPUBufferBinding binding{ .buffer = vBuffer.GetHandle() };
						SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
						const auto& iBuffer = e.m_MeshComp->m_Mesh->GetIndexBuffer();
						if (iBuffer)
						{
							binding.buffer = iBuffer.GetHandle();
							SDL_BindGPUIndexBuffer(pass, &binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
							SDL_DrawGPUIndexedPrimitives(
								pass,
								e.m_MeshComp->m_Mesh->GetNumIndices(),
								1,
								0,
								0,
								0);
						}
						else
						{
							SDL_DrawGPUPrimitives(
								pass,
								e.m_MeshComp->m_Mesh->GetNumVertices(),
								1,
								0,
								0);
						}
					}
				});
		}

		void Update(entt::registry& world, const apollo::GameTime&)
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
	};

	apollo::EAppResult Init(const apollo::EntryPoint&, apollo::App& app)
	{
		spdlog::set_level(spdlog::level::trace);

		auto* compRegistry = ecs::ComponentRegistry::GetInstance();
		compRegistry->RegisterComponent<MeshComponent>();
		compRegistry->RegisterComponent<GridComponent>();

		auto& renderer = *apollo::rdr::Context::GetInstance();
		auto& manager = *apollo::ecs::Manager::GetInstance();
		auto& camSystem = manager.AddSystem<apollo::demo::CameraSystem>(renderer.GetWindow());
		auto& mainSystem = manager.AddSystem<TestSystem>(app.GetMainWindow(), renderer, camSystem);

		mainSystem.m_Inspector.m_AssetManager = static_cast<editor::AssetManager*>(
			IAssetManager::GetInstance());
		auto& world = manager.GetEntityWorld();
		world.emplace<SceneSwitchRequestComponent>(
			world.create(),
			"01KMAX1C7SPE9BNEW848XEZT4D"_ulid);

		manager.AddSystem<GridDrawSystem>(
			renderer,
			mainSystem.m_TargetViewport,
			camSystem.GetCamera());
		manager.AddSystem<UiSystem>(
			renderer,
			ui::Context::s_Instance,
			rdr::ui::Renderer::s_Instance,
			mainSystem.m_TargetViewport);

		return EAppResult::Continue;
	}
} // namespace apollo::demo

apollo::EntryPoint apollo::GetEntryPoint(std::span<const char*>)
{
	return apollo::EntryPoint{
		.m_AppName = "Apollo Example",
		.m_OnInit = apollo::demo::Init,
	};
}