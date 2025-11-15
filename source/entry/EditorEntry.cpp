#include "Entry.hpp"

#include <asset/AssetManager.hpp>
#include <core/App.hpp>

#include <ecs/ComponentRegistry.hpp>
#include <editor/Editor.hpp>
#include <editor/asset/Manager.hpp>
#include <filesystem>

namespace apollo::editor {
	void RegisterComponents(ecs::ComponentRegistry& registry);
}

int main(int argc, const char** argv)
{
	using namespace apollo;

	// component registry is kinda special: the core engine runtime doesn't know about it, we need
	// to initialize it here
	auto& componentRegistry = apollo::ecs::ComponentRegistry::Init();
	apollo::editor::RegisterComponents(componentRegistry);

	EntryPoint entry{
		.m_Args = std::span{ argv, size_t(argc) },
		.m_AssetRoot = argc > 1 ? std::filesystem::absolute(argv[1])
								: std::filesystem::current_path(),
		.m_InitAssetManager = [](const std::filesystem::path& path,
								 rdr::GPUDevice& device,
								 mt::ThreadPool& tp) -> IAssetManager&
		{
			return IAssetManager::Init<editor::AssetManager>(path, device, tp);
		},
	};
	apollo::GetEntryPoint(entry);

	auto& app = apollo::App::Init(entry);

	apollo::EAppResult res = app.GetResultCode();

	if (res != apollo::EAppResult::Continue) [[unlikely]]
		goto APP_END;

	apollo::editor::Editor::Init(entry, app);

	res = app.Run();

APP_END:
	switch (res)
	{
	case apollo::EAppResult::Success: apollo::App::Shutdown(); return 0;
	default: apollo::App::Shutdown(); return 1;
	}
}