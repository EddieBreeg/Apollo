#include "Entry.hpp"

#include <asset/AssetManager.hpp>
#include <core/App.hpp>

#include <filesystem>

int main(int argc, const char** argv)
{
	brk::AssetManagerSettings assetManagerSettings{
		.m_AssetPath = std::filesystem::path(argv[0]).parent_path(),
	};

	brk::EntryPoint entry{ .m_AssetManagerSettings = assetManagerSettings };
	brk::GetEntryPoint(entry);
	entry.m_Args = std::span{ argv, size_t(argc) };

	auto& app = brk::App::Init(entry);

	brk::EAppResult res = app.GetResultCode();

	if (res != brk::EAppResult::Continue) [[unlikely]]
		goto APP_END;

	// not implemented yet
	// brk::AssetManager::GetInstance()->ImportMetadataBank();

	res = app.Run();

APP_END:
	switch (res)
	{
	case brk::EAppResult::Success: brk::App::Shutdown(); return 0;
	default: brk::App::Shutdown(); return 1;
	}
}