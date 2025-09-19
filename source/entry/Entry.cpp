#include "Entry.hpp"

#include <core/App.hpp>

int main(int argc, const char** argv)
{
	brk::EntryPoint entry;
	brk::GetEntryPoint(entry);
	entry.m_Args = std::span{ argv, size_t(argc) };

	auto& app = brk::App::Init(entry);

	brk::EAppResult res = app.GetResultCode();

	if (res == brk::EAppResult::Continue) [[likely]]
		res = app.Run();

	switch (res)
	{
	case brk::EAppResult::Success: brk::App::Shutdown(); return 0;
	default: brk::App::Shutdown(); return 1;
	}
}