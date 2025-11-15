#include "Editor.hpp"

#include <asset/AssetManager.hpp>
#include <core/App.hpp>
#include <core/Log.hpp>
#include <ecs/Manager.hpp>
#include <entry/Entry.hpp>
#include <imgui.h>
#include <rendering/Context.hpp>

namespace apollo::editor {
	Editor::Editor(std::span<const char*> args, rdr::Context& ctx)
		: m_RenderContext(ctx)
	{
		APOLLO_LOG_INFO("Initializing editor");
		if (args.size() < 2)
			return;

		m_ProjectPath = args[1];
		std::filesystem::current_path(m_ProjectPath);
		auto* assetManager = IAssetManager::GetInstance();
		APOLLO_ASSERT(assetManager, "Asset manager isn't initialized");
		assetManager->ImportMetadataBank();
	}

	EAppResult Editor::Init(const EntryPoint& entry, App& app)
	{
		auto& ecsManager = *ecs::Manager::GetInstance();
		ecsManager.AddSystem<Editor>(entry.m_Args, *app.GetRenderContext());
		return EAppResult::Continue;
	}

	void Editor::Update(entt::registry&, const GameTime&)
	{
		ImGui::Render();
		m_RenderContext.DrawImGuiLayer(rdr::ImGuiDrawCommand{ ImGui::GetDrawData() });
	}
} // namespace apollo::editor