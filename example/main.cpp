#include "DemoPCH.hpp"

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

	void ShaderParamWidget(
		const rdr::ShaderConstant& param,
		rdr::MaterialInstance& matInstance,
		uint32 blockIndex = 0)
	{
		using namespace apollo::rdr;
		const char* label = param.m_Name.c_str();

		switch (param.m_Type)
		{
		case ShaderConstant::Float:
			ImGui::DragFloat(
				label,
				&matInstance.GetFragmentConstant<float>(blockIndex, param.m_Offset));
			return;
		case ShaderConstant::Float2:
			ImGui::DragFloat2(
				label,
				&matInstance.GetFragmentConstant<float>(blockIndex, param.m_Offset));
			return;
		case ShaderConstant::Float3:
			ImGui::DragFloat3(
				label,
				&matInstance.GetFragmentConstant<float>(blockIndex, param.m_Offset));
			return;
		case ShaderConstant::Float4:
			ImGui::DragFloat4(
				label,
				&matInstance.GetFragmentConstant<float>(blockIndex, param.m_Offset));
			return;
		case ShaderConstant::Int: [[fallthrough]];
		case ShaderConstant::UInt:
			ImGui::DragInt(
				label,
				&matInstance.GetFragmentConstant<int32>(blockIndex, param.m_Offset));
			return;
		case ShaderConstant::Int2: [[fallthrough]];
		case ShaderConstant::UInt2:
			ImGui::DragInt2(
				label,
				&matInstance.GetFragmentConstant<int32>(blockIndex, param.m_Offset));
			return;
		case ShaderConstant::Int3: [[fallthrough]];
		case ShaderConstant::UInt3:
			ImGui::DragInt3(
				label,
				&matInstance.GetFragmentConstant<int32>(blockIndex, param.m_Offset));
			return;
		case ShaderConstant::Int4: [[fallthrough]];
		case ShaderConstant::UInt4:
			ImGui::DragInt4(
				label,
				&matInstance.GetFragmentConstant<int32>(blockIndex, param.m_Offset));
			return;
		default: ImGui::Text("%s (%s)", label, ShaderConstant::GetTypeName(param.m_Type)); return;
		}
	}

	void ShaderParamBlockWidget(rdr::MaterialInstance& matInstance, uint32 blockIndex = 0)
	{
		const auto* fragShader = matInstance.GetMaterial()->GetFragmentShader();
		const auto& block = fragShader->GetParameterBlocks()[blockIndex];
		if (block.m_Name.length())
			ImGui::SeparatorText(block.m_Name.c_str());
		else
			ImGui::Separator();

		ImGui::Text("Total size: %u", block.m_Size);

		for (uint32 i = 0; i < block.m_NumMembers; ++i)
		{
			ShaderParamWidget(block.m_Members[i], matInstance, blockIndex);
		}
	}

	struct MeshComponent
	{
		AssetRef<rdr::Mesh> m_Mesh;
		AssetRef<rdr::MaterialInstance> m_Material;

		static constexpr ecs::ComponentReflection<&MeshComponent::m_Mesh, &MeshComponent::m_Material>
			Reflection{
				"mesh",
				{ "mesh", "material" },
			};
	};

	struct TestSystem
	{
		Camera m_Camera;
		apollo::Window& m_Window;
		apollo::rdr::Context& m_RenderContext;
		Viewport m_TargetViewport;
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

		void DisplayUi()
		{
			m_TargetViewport.Update();

			ImGui::Begin("Camera Settings");
			ImGui::SliderFloat("Mouse Speed", &m_MouseSpeed, .0f, 10.0f);
			ImGui::SliderFloat("Move Speed", &m_MoveSpeed, .0f, 10.0f);
			ImGui::Text("Press F1 to unlock/relock camera");
			ImGui::End();

			// if (!mat)
			// 	return;

			// char title[37] = {};
			// mat->GetId().ToChars(title);

			// if (!mat->IsLoaded())
			// 	return;

			// ImGui::Begin(title);

			// ImGui::SeparatorText("Fragment Shader Constants");
			// ShaderParamBlockWidget(*mat);
			// ImGui::End();
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

			auto* swapchainTexture = m_RenderContext.GetSwapchainTexture();

			if (!swapchainTexture)
				return;

			ProcessInputs(world, time);
			DisplayUi();
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