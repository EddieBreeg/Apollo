#include "Entry.hpp"

#include <asset/AssetManager.hpp>
#include <core/App.hpp>

#include <filesystem>

#include <editor/asset/AssetLoaders.hpp>
#include <editor/Editor.hpp>

int main(int argc, const char** argv)
{
	brk::AssetManagerSettings assetManagerSettings{
		.m_AssetPath = argc > 1 ? std::filesystem::absolute(argv[1])
								: std::filesystem::current_path(),
	};
	assetManagerSettings.m_MetadataImportFunc = &brk::editor::ImporteAssetMetadata;

	assetManagerSettings.m_ImportTexture2d = brk::editor::ImportTexture2d;
	assetManagerSettings.m_LoadShader = brk::editor::LoadShader;
	assetManagerSettings.m_LoadMaterial = &brk::editor::LoadMaterial;
	assetManagerSettings.m_LoadFont = &brk::editor::LoadFont;

	brk::EntryPoint entry{ .m_AssetManagerSettings = assetManagerSettings };
	brk::GetEntryPoint(entry);
	entry.m_Args = std::span{ argv, size_t(argc) };

	auto& app = brk::App::Init(entry);

	brk::EAppResult res = app.GetResultCode();

	if (res != brk::EAppResult::Continue) [[unlikely]]
		goto APP_END;

	brk::editor::Editor::Init(entry, app);

	res = app.Run();

APP_END:
	switch (res)
	{
	case brk::EAppResult::Success: brk::App::Shutdown(); return 0;
	default: brk::App::Shutdown(); return 1;
	}
}