#include <entry/Entry.hpp>

namespace  {
	struct Demo : public IGameState
	{
		EAppResult OnInit(const apollo::EntryPoint&, apollo::App& app) override
		{
			/*
			Initialization code goes here
			...
			*/
			return EAppResult::Continue;
		}

		void OnQuit(App&) override
		{
			APOLLO_LOG_INFO("Quitting game");
		}

		/*
		Game data goes here
		...
		*/
	};
} // namespace apollo::demo

apollo::EntryPoint apollo::GetEntryPoint(std::span<const char*>)
{
	return apollo::EntryPoint{
		.m_AppName = "My Game",
		.m_GameState = std::make_unique<Demo>(),
	};
}