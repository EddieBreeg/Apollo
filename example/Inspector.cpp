#include "Inspector.hpp"
#include "DemoPCH.hpp"
#include <editor/asset/Manager.hpp>
#include <imgui.h>

namespace {
	const apollo::GameObject* Outliner(
		const apollo::ULIDMap<apollo::GameObject>& objects,
		const apollo::GameObject* current)
	{
		if (!ImGui::BeginListBox("Selected Object"))
			return current;

		const auto* selection = current;

		for (auto it = objects.begin(); it != objects.end(); ++it)
		{
			const auto& object = it->second;
			if (ImGui::Selectable(object.m_Name.c_str(), &object == current))
			{
				selection = &object;
				break;
			}
		}

		ImGui::EndListBox();
		return selection;
	}

	void ShaderParamWidget(
		const apollo::rdr::ShaderConstant& param,
		apollo::rdr::MaterialInstance& matInstance,
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

	void ShaderParamBlockWidget(
		apollo::rdr::MaterialInstance& matInstance,
		const apollo::rdr::ShaderConstantBlock& block,
		uint32 blockIndex = 0)
	{
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

	void MaterialWidget(
		apollo::rdr::MaterialInstance& material,
		apollo::editor::AssetManager* assetManager)
	{
		if (!material.IsLoaded())
			return;

		if (!ImGui::TreeNode("Material Info"))
			return;

		const auto key = material.GetKey();
		{
			bool temp = key.WritesToDepthBuffer();
			ImGui::Checkbox("Depth Write Enabled", &temp);
		}
		ImGui::Text(
			"Material Index: %u\nInstance Index: %u",
			key.GetMaterialIndex(),
			key.GetInstanceIndex());
		const auto* fragShader = material.GetMaterial()->GetFragmentShader();
		const std::span blocks = fragShader->GetParameterBlocks();
		for (uint32 i = 0; i < blocks.size(); ++i)
		{
			ShaderParamBlockWidget(material, blocks[i], i);
		}

		if (assetManager && ImGui::Button("Reload Material"))
		{
			auto* mat = material.GetMaterial();
			assetManager->RequestReload(*mat->GetVertexShader());
			assetManager->RequestReload(*mat->GetFragmentShader());
			assetManager->RequestReload(*mat);
		}

		ImGui::TreePop();
	}

	bool TransformWidget(apollo::TransformComponent& comp, float3& eulerAngles, bool reset = false)
	{
		if (reset)
		{
			eulerAngles = glm::eulerAngles(comp.m_Rotation);
		}

		if (!ImGui::TreeNode("Transform"))
		{
			return false;
		}
		bool res = ImGui::DragFloat3("Position", &comp.m_Position.x, 0.1f);
		res |= ImGui::DragFloat3("Scale", &comp.m_Scale.x, 0.1f);
		if (ImGui::DragFloat3("Rotation", &eulerAngles.x, 0.01f))
		{
			res = true;
			comp.m_Rotation = glm::quat{ eulerAngles };
		}
		ImGui::TreePop();
		return res;
	}
} // namespace

namespace apollo::demo {
	void Inspector::Update(entt::registry& world)
	{
		if (!m_Scene || !m_Scene->IsLoaded() || !m_ShowInspector)
			return;

		if (!ImGui::Begin("Inspector", &m_ShowInspector))
		{
			ImGui::End();
			return;
		}

		const auto* selection = Outliner(m_Scene->GetGameObjects(), m_CurrentObject);
		const bool selectionChanged = selection != m_CurrentObject;
		m_CurrentObject = selection;
		if (!selection)
		{
			ImGui::End();
			return;
		}

		if (TransformComponent* transform = world.try_get<TransformComponent>(selection->m_Entity))
			TransformWidget(*transform, m_Euler, selectionChanged);

		if (MeshComponent* meshComp = world.try_get<MeshComponent>(selection->m_Entity);
			meshComp && meshComp->m_Material)
		{
			MaterialWidget(*meshComp->m_Material, m_AssetManager);
		}

		ImGui::End();
	}
} // namespace apollo::demo