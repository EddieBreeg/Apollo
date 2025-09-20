#include "Editor.hpp"
#include <core/Log.hpp>
#include <ecs/Manager.hpp>
#include <entry/Entry.hpp>

namespace brk::editor {
	Editor::Editor(std::span<const char*>)
	{
		BRK_LOG_INFO("Initializing editor");
	}

	EAppResult Editor::Init(const EntryPoint& entry, App&)
	{
		auto& ecsManager = *ecs::Manager::GetInstance();
		ecsManager.AddSystem<Editor>(entry.m_Args);
		return EAppResult::Continue;
	}

	void Editor::Update() {}
} // namespace brk::editor