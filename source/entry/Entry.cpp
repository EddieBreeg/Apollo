#include "Entry.hpp"

#include <asset/AssetManager.hpp>
#include <core/App.hpp>

#include <filesystem>

#ifdef BRK_EDITOR
#include <editor/AssetImporters.hpp>
#include <editor/Editor.hpp>
#endif

int main(int argc, const char** argv)
{
	brk::AssetManagerSettings assetManagerSettings{
		.m_AssetPath = argc > 1 ? std::filesystem::absolute(argv[1])
								: std::filesystem::current_path(),
	};
#ifdef BRK_EDITOR
	assetManagerSettings.m_MetadataImportFunc = &brk::editor::ImporteAssetMetadata;

	assetManagerSettings.m_ImportTexture2d = brk::editor::ImportTexture2d;
#endif

	brk::EntryPoint entry{ .m_AssetManagerSettings = assetManagerSettings };
	brk::GetEntryPoint(entry);
	entry.m_Args = std::span{ argv, size_t(argc) };

	auto& app = brk::App::Init(entry);

	brk::EAppResult res = app.GetResultCode();

	if (res != brk::EAppResult::Continue) [[unlikely]]
		goto APP_END;

#ifdef BRK_EDITOR
	brk::editor::Editor::Init(entry, app);
#else
	brk::AssetManager::GetInstance()->ImportMetadataBank();
#endif

	res = app.Run();

APP_END:
	switch (res)
	{
	case brk::EAppResult::Success: brk::App::Shutdown(); return 0;
	default: brk::App::Shutdown(); return 1;
	}
}