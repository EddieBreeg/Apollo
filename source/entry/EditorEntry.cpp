#include "Entry.hpp"

#include <asset/AssetManager.hpp>
#include <core/App.hpp>

#include <filesystem>

#include <editor/Editor.hpp>
#include <editor/asset/AssetLoaders.hpp>

int main(int argc, const char** argv)
{
	apollo::AssetManagerSettings assetManagerSettings{
		.m_AssetPath = argc > 1 ? std::filesystem::absolute(argv[1])
								: std::filesystem::current_path(),
	};
	assetManagerSettings.m_MetadataImportFunc = &apollo::editor::ImporteAssetMetadata;

	assetManagerSettings.m_LoadTexture2d = apollo::editor::LoadTexture2d;
	assetManagerSettings.m_LoadShader = apollo::editor::LoadShader;
	assetManagerSettings.m_LoadMaterial = &apollo::editor::LoadMaterial;
	assetManagerSettings.m_LoadFont = &apollo::editor::LoadFont;

	apollo::EntryPoint entry{ .m_AssetManagerSettings = assetManagerSettings };
	apollo::GetEntryPoint(entry);
	entry.m_Args = std::span{ argv, size_t(argc) };

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