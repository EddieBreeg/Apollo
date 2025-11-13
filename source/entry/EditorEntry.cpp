#include "Entry.hpp"

#include <asset/AssetManager.hpp>
#include <core/App.hpp>

#include <ecs/ComponentRegistry.hpp>
#include <editor/Editor.hpp>
#include <editor/asset/AssetLoaders.hpp>
#include <filesystem>

namespace apollo::editor {
	void RegisterComponents(ecs::ComponentRegistry& registry);
}

int main(int argc, const char** argv)
{
	apollo::AssetManagerSettings assetManagerSettings{
		.m_AssetPath = argc > 1 ? std::filesystem::absolute(argv[1])
								: std::filesystem::current_path(),
	};
	assetManagerSettings.m_MetadataImportFunc = &apollo::editor::ImporteAssetMetadata;

	{
		using namespace apollo;
		using namespace apollo::editor;
		using namespace apollo::rdr;
		assetManagerSettings.m_LoadTexture2d = &AssetHelper<Texture2D>::Load;
		assetManagerSettings.m_LoadVertexShader = &AssetHelper<VertexShader>::Load;
		assetManagerSettings.m_LoadFragmentShader = &AssetHelper<FragmentShader>::Load;
		assetManagerSettings.m_LoadMaterial = &AssetHelper<Material>::Load;
		assetManagerSettings.m_LoadMaterialInstance = &AssetHelper<MaterialInstance>::Load;
		assetManagerSettings.m_LoadMesh = &AssetHelper<Mesh>::Load;
		assetManagerSettings.m_LoadFont = &AssetHelper<txt::FontAtlas>::Load;
		assetManagerSettings.m_LoadScene = &AssetHelper<Scene>::Load;
	}

	// component registry is kinda special: the core engine runtime doesn't know about it, we need
	// to initialize it here
	auto& componentRegistry = apollo::ecs::ComponentRegistry::Init();
	apollo::editor::RegisterComponents(componentRegistry);

	apollo::EntryPoint entry{
		.m_Args = std::span{ argv, size_t(argc) },
		.m_AssetManagerSettings = assetManagerSettings,
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