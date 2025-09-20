#include "Entry.hpp"

#include <core/App.hpp>
#ifdef BRK_EDITOR
#include <editor/Editor.hpp>
#endif

int main(int argc, const char** argv)
{
	brk::EntryPoint entry;
	brk::GetEntryPoint(entry);
	entry.m_Args = std::span{ argv, size_t(argc) };

	auto& app = brk::App::Init(entry);

	brk::EAppResult res = app.GetResultCode();

	if (res != brk::EAppResult::Continue) [[unlikely]]
		goto APP_END;

#ifdef BRK_EDITOR
	brk::editor::Editor::Init(entry, app);
#endif

	res = app.Run();

APP_END:
	switch (res)
	{
	case brk::EAppResult::Success: brk::App::Shutdown(); return 0;
	default: brk::App::Shutdown(); return 1;
	}
}