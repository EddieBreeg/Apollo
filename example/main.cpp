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

	template <class UInt, uint8 P>
	[[nodiscard]] UInt ToFixedPoint(float x)
	{
		const UInt pow2 = UInt(1) << P;
		constexpr UInt mask = pow2 - 1;
		const float rounding = x < 0 ? -0.5f : 0.5f;

		return static_cast<UInt>((x * pow2 + rounding)) & mask;
	}

	uint64 CreateSortKey(
		const rdr::MaterialInstance& mat,
		float3 pos,
		float3 camPos,
		float3 viewVector)
	{
		const rdr::MaterialInstanceKey matKey = mat.GetKey();
		const float distance = glm::dot(pos - camPos, viewVector) / 100.f;
		if (matKey.WritesToDepthBuffer())
		{
			return ((uint64)matKey << 24) | ToFixedPoint<uint64, 24>(distance);
		}
		return BIT(54) | (ToFixedPoint<uint64, 24>(-distance) << 30) | (uint64)matKey;
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

		void ProcessInputs(
			entt::registry& world,
			const GameTime& time,
			const float3 fwd,
			const float3 right)
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
					m_Camera.m_Translate += right * moveSpeed * lateralMovement;
				}
				if (fwdMovement)
				{
					m_Camera.m_Translate += fwd * moveSpeed * fwdMovement;
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

			ImGui::Begin("Settings & Info");
			ImGui::SliderFloat("Mouse Speed", &m_MouseSpeed, .0f, 10.0f);
			ImGui::SliderFloat("Move Speed", &m_MoveSpeed, .0f, 10.0f);
			ImGui::Text("Press F1 to unlock/relock camera");

			ImGui::Checkbox("Show Inspector", &m_Inspector.m_ShowInspector);
			ImGui::Checkbox("Show Depth/Stencil Content", &m_TargetViewport.m_ShowDepth);

			ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);
			ImGui::End();
		}

		void EmitGPUCommands(const entt::registry& world, float3 viewVector)
		{
			m_RenderContext.PushVertexShaderConstants(m_CamMatrix);
			m_RenderContext.BeginRenderPass(m_RenderPass);
			m_RenderContext.SetViewport(m_TargetViewport.m_Rectangle);
			const auto view = world.view<const MeshComponent, const TransformComponent>();

			using ValT = std::pair<const MeshComponent*, const TransformComponent*>;
			std::vector<ValT> meshes;
			meshes.reserve(view.size_hint());

			for (const auto entt : view)
			{
				const auto& mesh = view.get<const MeshComponent>(entt);
				if (!mesh.m_Mesh || !mesh.m_Material || !mesh.m_Mesh->IsLoaded() ||
					!mesh.m_Material->IsLoaded())
					continue;

				const auto& transform = view.get<const TransformComponent>(entt);
				meshes.emplace_back(&mesh, &transform);
			}
			std::sort(
				meshes.begin(),
				meshes.end(),
				[viewVector, camPos = m_Camera.m_Translate](const ValT& lhs, const ValT& rhs)
				{
					const uint64 k1 = CreateSortKey(
						*lhs.first->m_Material,
						lhs.second->m_Position,
						camPos,
						viewVector);
					const uint64 k2 = CreateSortKey(
						*rhs.first->m_Material,
						rhs.second->m_Position,
						camPos,
						viewVector);
					return k1 < k2;
				});
			for (const auto [mesh, transform] : meshes)
			{
				m_RenderContext.BindMaterialInstance(*mesh->m_Material);
				const auto modelMat = ComputeTransformMatrix(
					transform->m_Position,
					transform->m_Scale,
					transform->m_Rotation);
				m_RenderContext.PushVertexShaderConstants(modelMat, 1);
				m_RenderContext.BindVertexBuffer(mesh->m_Mesh->GetVertexBuffer());

				const rdr::Buffer& iBuffer = mesh->m_Mesh->GetIndexBuffer();

				if (iBuffer)
				{
					m_RenderContext.BindIndexBuffer(iBuffer);
					m_RenderContext.DrawIndexedPrimitives(
						rdr::IndexedDrawCall{
							.m_NumIndices = mesh->m_Mesh->GetNumIndices(),
						});
				}
				else
				{
					m_RenderContext.DrawPrimitives(
						rdr::DrawCall{ .m_NumVertices = mesh->m_Mesh->GetNumVertices() });
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

			const float3 fwdVec = m_Camera.GetForward();
			const float3 rightVec = m_Camera.GetRight();

			ProcessInputs(world, time, fwdVec, rightVec);
			DisplayUi(world);
			m_CamMatrix = GetProjMatrix(m_TargetViewport) * m_CamMatrix;

			EmitGPUCommands(world, fwdVec);
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