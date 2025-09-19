#include "Entry.hpp"

int main(int argc, const char** argv)
{
	brk::EntryPoint entry;
	brk::GetEntryPoint(entry);
	entry.m_Args = std::span{ argv, size_t(argc) };

	if (entry.m_OnInit)
		entry.m_OnInit(entry);
}