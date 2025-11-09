#include "DemoPCH.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

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
		AssetRef<rdr::MaterialInstance> m_Material;
		rdr::Buffer m_VBuffer;
		rdr::Buffer m_IBuffer;
		uint32 m_NumIndices = 0;

		static constexpr ecs::ComponentReflection<&MeshComponent::m_Material> Reflection{
			"mesh",
			{ "material" },
		};
	};

	bool LoadMesh(SDL_GPUCopyPass* copyPass, const char* path, MeshComponent& out_mesh)
	{
		Assimp::Importer importer;
		const auto* scene = importer.ReadFile(
			path,
			aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_FixInfacingNormals |
				aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph | aiProcess_FlipUVs);
		if (!scene)
		{
			APOLLO_LOG_ERROR("Failed to load mesh from {}: {}", path, importer.GetErrorString());
			return false;
		}

		if (!scene->HasMeshes())
			return false;

		const aiMesh* am = scene->mMeshes[0];

		std::vector<rdr::Vertex3d> vertices;
		vertices.reserve(am->mNumVertices);
		std::vector<uint32> indices;
		indices.reserve(am->mNumFaces * 3);

		for (uint32 i = 0; i < am->mNumVertices; ++i)
		{
			const float3 pos{ am->mVertices[i].x, am->mVertices[i].y, am->mVertices[i].z };
			const float3 nor{ am->mNormals[i].x, am->mNormals[i].y, am->mNormals[i].z };
			const float2 uv{ am->mTextureCoords[0][i].x, am->mTextureCoords[0][i].y };
			vertices.emplace_back(rdr::Vertex3d{ pos, nor, uv });
		}

		for (uint32 i = 0; i < am->mNumFaces; ++i)
		{
			const auto& face = am->mFaces[i];
			for (uint32 j = 0; j < face.mNumIndices; ++j)
			{
				indices.emplace_back(face.mIndices[j]);
			}
		}
		const uint32 vertSize = am->mNumVertices * sizeof(rdr::Vertex3d);
		out_mesh.m_NumIndices = (uint32)indices.size();
		const uint32 indSize = 4 * out_mesh.m_NumIndices;

		out_mesh.m_VBuffer = rdr::Buffer(rdr::EBufferFlags::Vertex, vertSize);
		out_mesh.m_IBuffer = rdr::Buffer(rdr::EBufferFlags::Index, 4 * (uint32)indices.size());
		out_mesh.m_VBuffer.UploadData(copyPass, vertices.data(), vertSize);
		out_mesh.m_IBuffer.UploadData(copyPass, indices.data(), indSize);
		return true;
	}

	struct TestSystem
	{
		Camera m_Camera;
		apollo::Window& m_Window;
		apollo::rdr::Context& m_RenderContext;
		apollo::AssetRef<Scene> m_Scene;
		apollo::AssetRef<rdr::MaterialInstance> m_Material;
		Viewport m_TargetViewport;
		rdr::RenderPass m_RenderPass;
		bool m_AssetsReady = false;

		float m_AntiAliasing = 1.0f;
		float m_MouseSpeed = 3.0f;
		float m_MoveSpeed = 1.0f;
		bool m_CameraLocked = true;
		glm::mat4x4 m_CamMatrix = glm::identity<glm::mat4x4>();
		glm::mat4x4 m_ModelMatrix;

		void ProcessSceneLoadFinished(
			entt::registry& world,
			entt::entity sceneEntity,
			SDL_GPUCopyPass* copyPass)
		{
			const auto* sceneComp = world.try_get<const SceneComponent>(sceneEntity);
			DEBUG_CHECK(sceneComp)
			{
				APOLLO_LOG_ERROR("Missing SceneComponent");
				return;
			}
			m_Scene = sceneComp->m_Scene;
			const GameObject* object = m_Scene->GetGameObject("01K8EC89XVHA1N3ZRZTFGNMYT5"_ulid);
			auto& mesh = world.get<MeshComponent>(object->m_Entity);
			m_Material = mesh.m_Material;
			if (!m_Material || !m_Material->IsLoaded())
				return;

			m_AssetsReady = LoadMesh(copyPass, "assets/Cube.obj", mesh);
		}

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

		void DisplayUi(rdr::MaterialInstance* mat)
		{
			m_TargetViewport.Update();

			ImGui::Begin("Camera Settings");
			ImGui::SliderFloat("Mouse Speed", &m_MouseSpeed, .0f, 10.0f);
			ImGui::SliderFloat("Move Speed", &m_MoveSpeed, .0f, 10.0f);
			ImGui::Text("Press F1 to unlock/relock camera");
			ImGui::End();

			if (!mat)
				return;

			char title[37] = {};
			mat->GetId().ToChars(title);

			if (!mat->IsLoaded())
				return;

			ImGui::Begin(title);

			ImGui::SeparatorText("Fragment Shader Constants");
			ShaderParamBlockWidget(*mat);
			ImGui::End();
		}

		void EmitGPUCommands(const entt::registry& world)
		{
			m_RenderContext.PushVertexShaderConstants(m_CamMatrix);
			m_RenderContext.BeginRenderPass(m_RenderPass);
			m_RenderContext.SetViewport(m_TargetViewport.m_Rectangle);

			m_RenderContext.BindMaterialInstance(*m_Material);

			const auto view = world.view<const MeshComponent, const TransformComponent>();
			for (const auto entt : view)
			{
				const auto& mesh = view.get<const MeshComponent>(entt);
				const auto& transform = view.get<const TransformComponent>(entt);
				const auto modelMat = ComputeTransformMatrix(
					transform.m_Position,
					transform.m_Scale,
					transform.m_Rotation);
				m_RenderContext.PushVertexShaderConstants(modelMat, 1);
				m_RenderContext.BindVertexBuffer(mesh.m_VBuffer);

				if (mesh.m_IBuffer)
				{
					m_RenderContext.BindIndexBuffer(mesh.m_IBuffer);
				}
				m_RenderContext.DrawIndexedPrimitives(
					rdr::IndexedDrawCall{
						.m_NumIndices = mesh.m_NumIndices,
					});
			}
		}

		void Update(entt::registry& world, const apollo::GameTime& time)
		{
			if (!m_Window) [[unlikely]]
				return;

			auto* swapchainTexture = m_RenderContext.GetSwapchainTexture();
			auto* mainCommandBuffer = m_RenderContext.GetMainCommandBuffer();

			if (!m_AssetsReady)
			{
				const auto view = world.view<const SceneLoadFinishedEventComponent>();
				if (!view.empty())
				{
					SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(mainCommandBuffer);
					ProcessSceneLoadFinished(world, *view->begin(), copyPass);
					SDL_EndGPUCopyPass(copyPass);
				}
			}

			if (!swapchainTexture)
				return;

			ProcessInputs(world, time);
			DisplayUi(m_Material.Get());
			m_CamMatrix = GetProjMatrix(m_TargetViewport) * m_CamMatrix;

			if (!m_AssetsReady)
				return;

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