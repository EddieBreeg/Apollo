#include "Entry.hpp"

#include <asset/AssetManager.hpp>
#include <core/App.hpp>

#include <filesystem>

int main(int argc, const char** argv)
{
	apollo::AssetManagerSettings assetManagerSettings{
		.m_AssetPath = std::filesystem::path(argv[0]).parent_path(),
	};

	apollo::EntryPoint entry{
		.m_Args = std::span{ argv, size_t(argc) },
		.m_AssetManagerSettings = assetManagerSettings,
	};
	apollo::GetEntryPoint(entry);

	auto& app = apollo::App::Init(entry);

	apollo::EAppResult res = app.GetResultCode();

	if (res != apollo::EAppResult::Continue) [[unlikely]]
		goto APP_END;

	// not implemented yet
	// apollo::AssetManager::GetInstance()->ImportMetadataBank();

	res = app.Run();

APP_END:
	switch (res)
	{
	case apollo::EAppResult::Success: apollo::App::Shutdown(); return 0;
	default: apollo::App::Shutdown(); return 1;
	}
}