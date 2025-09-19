#include <core/Log.hpp>
#include <entry/Entry.hpp>

void brk::GetEntryPoint(EntryPoint& out_entry)
{
	out_entry.m_AppName = "Breakout Example";
	out_entry.m_OnInit = [](const EntryPoint& entry)
	{
		BRK_LOG_INFO("Hello from {}!", entry.m_AppName);
		return EAppResult::Continue;
	};
}