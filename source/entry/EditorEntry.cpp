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

	const std::span appArgs{ argv, static_cast<size_t>(argc) };
	EntryPoint entry = apollo::GetEntryPoint(appArgs);
	if (entry.m_AssetRoot.empty())
	{
		const std::filesystem::path path = argc > 1 ? std::filesystem::absolute(argv[1])
													: std::filesystem::current_path();
		entry.m_AssetRoot = path.string();
	}

	auto& app = apollo::App::Init(
		entry,
		[](const std::string& path, rdr::GPUDevice& device, mt::ThreadPool& tp) -> IAssetManager&
		{
			return IAssetManager::Init<editor::AssetManager>(path, device, tp);
		});

	apollo::EAppResult res = app.GetResultCode();

	if (res != apollo::EAppResult::Continue) [[unlikely]]
		goto APP_END;

	apollo::editor::Editor::Init(appArgs, app);

	res = app.Run();

APP_END:
	switch (res)
	{
	case apollo::EAppResult::Success: apollo::App::Shutdown(); return 0;
	default: apollo::App::Shutdown(); return 1;
	}
}