#include "Entry.hpp"

#include <asset/AssetManager.hpp>
#include <core/App.hpp>

#include <filesystem>

int main(int argc, const char** argv)
{
	const std::span appArgs{ argv, static_cast<size_t>(argc) };
	apollo::EntryPoint ep = apollo::GetEntryPoint(appArgs);

	// note: this crashes, because right now we literally haven't implemented the asset manager for
	// the game
	auto& app = apollo::App::Init(ep, nullptr);

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