#include <entry/Entry.hpp>
#include <iostream>

void brk::GetEntryPoint(EntryPoint& out_entry)
{
	out_entry.m_AppName = "Breakout Example";
	out_entry.m_OnInit = [](const EntryPoint& entry)
	{
		std::cout << "Hello from " << entry.m_AppName << "!\n";
	};
}