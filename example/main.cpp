#include "DemoPCH.hpp"
#include "Inspector.hpp"

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

	struct TestSystem
	{
		Camera m_Camera;
		apollo::Window& m_Window;
		apollo::rdr::Context& m_RenderContext;
		Viewport m_TargetViewport;
		Inspector m_Inspector;
		rdr::RenderPass m_RenderPass;

		float m_AntiAliasing = 1.0f;
		float m_MouseSpeed = 3.0f;
		float m_MoveSpeed = 1.0f;
		bool m_CameraLocked = true;
		glm::mat4x4 m_CamMatrix = glm::identity<glm::mat4x4>();
		glm::mat4x4 m_ModelMatrix;

		void ProcessInputs(entt::registry& world, const GameTime& time)
		{
			using namespace apollo::inputs;

			float2 mouseMotion = {};
			constexpr float maxPitch = .49f * std::numbers::pi_v<float>;

			{
				const auto view = world.view<const inputs::KeyDownEventComponent>();
				for (const auto e : view)
				{
					const auto& comp = view->get(e);
					switch (comp.m_Key)
					{
					case inputs::EKey::F1:
						if (comp.m_Repeat)
							break;

						DEBUG_CHECK(
							SDL_SetWindowRelativeMouseMode(m_Window.GetHandle(), m_CameraLocked))
						{
							APOLLO_LOG_ERROR(
								"Failed to set relative cursor mode: {}",
								SDL_GetError());
						}
						m_CameraLocked = !m_CameraLocked;
						break;
					default: break;
					}
				}
				const bool* keyStates = inputs::GetKeyStates();
				const float lateralMovement = float(
					keyStates[SDL_SCANCODE_D] - keyStates[SDL_SCANCODE_A]);
				const float fwdMovement = float(
					keyStates[SDL_SCANCODE_W] - keyStates[SDL_SCANCODE_S]);
				const float vMovement = float(
					keyStates[SDL_SCANCODE_Q] - keyStates[SDL_SCANCODE_E]);

				const float moveSpeed = time.GetDelta().count() * m_MoveSpeed;

				if (lateralMovement)
				{
					m_Camera.m_Translate += m_Camera.GetRight() * moveSpeed * lateralMovement;
				}
				if (fwdMovement)
				{
					m_Camera.m_Translate += m_Camera.GetForward() * moveSpeed * fwdMovement;
				}
				if (vMovement)
				{
					m_Camera.m_Translate += moveSpeed * vMovement * float3{ 0, 1, 0 };
				}
			}

			if (!m_CameraLocked)
			{
				const auto view = world.view<inputs::MouseMotionEventComponent>();
				for (const auto e : view)
				{
					mouseMotion += view->get(e).m_Motion;
				}
				const float camSpeed = time.GetDelta().count() * m_MouseSpeed;
				m_Camera.m_Yaw -= camSpeed * mouseMotion.x;
				m_Camera.m_Pitch = Clamp(
					m_Camera.m_Pitch - camSpeed * mouseMotion.y,
					-maxPitch,
					maxPitch);
			}

			m_CamMatrix = m_Camera.GetLookAt();
		}

		TestSystem(apollo::Window& window, apollo::rdr::Context& renderer)
			: m_Window(window)
			, m_RenderContext(renderer)
			, m_RenderPass(
				  rdr::RenderPassSettings{
					  .m_NumColorTargets = 1,
					  .m_ColorTargets = { rdr::ColorTargetSettings{
						  .m_Texture = &m_TargetViewport.m_ColorTarget,
						  .m_ClearColor = { .1f, .1f, .1f, 1.0f },
					  } },
					  .m_DepthStencilTarget =
						  rdr::DepthStencilTargetSettings{
							  .m_Texture = &m_TargetViewport.m_DepthStencilTarget,
						  },
				  })
		{
			m_ModelMatrix = glm::translate(glm::identity<glm::mat4x4>(), { 0, 0, -1 });
		}

		~TestSystem() {}

		void DisplayUi(entt::registry& world)
		{
			m_TargetViewport.Update();
			m_Inspector.Update(world);

			ImGui::Begin("Settings");
			ImGui::SliderFloat("Mouse Speed", &m_MouseSpeed, .0f, 10.0f);
			ImGui::SliderFloat("Move Speed", &m_MoveSpeed, .0f, 10.0f);
			ImGui::Text("Press F1 to unlock/relock camera");

			ImGui::Checkbox("Show Inspector", &m_Inspector.m_ShowInspector);
			ImGui::Checkbox("Show Depth/Stencil Content", &m_TargetViewport.m_ShowDepth);
			ImGui::End();
		}

		void EmitGPUCommands(const entt::registry& world)
		{
			m_RenderContext.PushVertexShaderConstants(m_CamMatrix);
			m_RenderContext.BeginRenderPass(m_RenderPass);
			m_RenderContext.SetViewport(m_TargetViewport.m_Rectangle);

			const auto view = world.view<const MeshComponent, const TransformComponent>();
			for (const auto entt : view)
			{
				const auto& mesh = view.get<const MeshComponent>(entt);
				if (!mesh.m_Mesh || !mesh.m_Material || !mesh.m_Mesh->IsLoaded() ||
					!mesh.m_Material->IsLoaded())
					continue;

				m_RenderContext.BindMaterialInstance(*mesh.m_Material);
				const auto& transform = view.get<const TransformComponent>(entt);
				const auto modelMat = ComputeTransformMatrix(
					transform.m_Position,
					transform.m_Scale,
					transform.m_Rotation);
				m_RenderContext.PushVertexShaderConstants(modelMat, 1);
				m_RenderContext.BindVertexBuffer(mesh.m_Mesh->GetVertexBuffer());

				const rdr::Buffer& iBuffer = mesh.m_Mesh->GetIndexBuffer();

				if (iBuffer)
				{
					m_RenderContext.BindIndexBuffer(iBuffer);
					m_RenderContext.DrawIndexedPrimitives(
						rdr::IndexedDrawCall{
							.m_NumIndices = mesh.m_Mesh->GetNumIndices(),
						});
				}
				else
				{
					m_RenderContext.DrawPrimitives(
						rdr::DrawCall{ .m_NumVertices = mesh.m_Mesh->GetNumVertices() });
				}
			}
		}

		void Update(entt::registry& world, const apollo::GameTime& time)
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

			ProcessInputs(world, time);
			DisplayUi(world);
			m_CamMatrix = GetProjMatrix(m_TargetViewport) * m_CamMatrix;

			EmitGPUCommands(world);
		}
	};

	apollo::EAppResult Init(const apollo::EntryPoint&, apollo::App& app)
	{
		spdlog::set_level(spdlog::level::trace);

		auto* compRegistry = ecs::ComponentRegistry::GetInstance();
		compRegistry->RegisterComponent<MeshComponent>();

		auto& renderer = *apollo::rdr::Context::GetInstance();
		ImGui::SetCurrentContext(app.GetImGuiContext());
		auto& manager = *apollo::ecs::Manager::GetInstance();
		manager.AddSystem<TestSystem>(app.GetMainWindow(), renderer);
		auto& world = manager.GetEntityWorld();
		world.emplace<SceneSwitchRequestComponent>(
			world.create(),
			"01K7VZZSR16FXR2NF8DNYSJQQ4"_ulid);

		return EAppResult::Continue;
	}
} // namespace apollo::demo

void apollo::GetEntryPoint(EntryPoint& out_entry)
{
	out_entry.m_AppName = "Apollo Example";
	out_entry.m_OnInit = apollo::demo::Init;
}