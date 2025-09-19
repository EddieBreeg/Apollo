#pragma once

#include <PCH.hpp>
#include <span>

namespace brk {
	struct EntryPoint
	{
		const char* m_AppName = nullptr;
		uint32 m_WindowWidth = 0;
		uint32 m_WindowHeight = 0;

		std::span<const char*> m_Args;

		void (*m_OnInit)(const EntryPoint&);
	};

	extern void GetEntryPoint(EntryPoint& out_entry);
} // namespace brk