#include "Editor.hpp"

#include <asset/AssetManager.hpp>
#include <core/Log.hpp>
#include <ecs/Manager.hpp>
#include <entry/Entry.hpp>

namespace apollo::editor {
	Editor::Editor(std::span<const char*> args)
	{
		APOLLO_LOG_INFO("Initializing editor");
		if (args.size() < 2)
			return;

		m_ProjectPath = args[1];
		std::filesystem::current_path(m_ProjectPath);
		auto* assetManager = AssetManager::GetInstance();
		APOLLO_ASSERT(assetManager, "Asset manager isn't initialized");
		assetManager->ImportMetadataBank();
	}

	EAppResult Editor::Init(const EntryPoint& entry, App&)
	{
		auto& ecsManager = *ecs::Manager::GetInstance();
		ecsManager.AddSystem<Editor>(entry.m_Args);
		return EAppResult::Continue;
	}

	void Editor::Update(entt::registry&, const GameTime&) {}
} // namespace apollo::editor