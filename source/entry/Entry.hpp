#pragma once

#include <PCH.hpp>
#include <span>

namespace brk {
	enum class EAppResult: int8
	{
		Continue,
		Success,
		Failure,
	};

	struct EntryPoint
	{
		const char* m_AppName = nullptr;
		uint32 m_WindowWidth = 1280;
		uint32 m_WindowHeight = 720;

		std::span<const char*> m_Args;

		EAppResult (*m_OnInit)(const EntryPoint&);
	};

	extern void GetEntryPoint(EntryPoint& out_entry);
} // namespace brk