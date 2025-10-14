#pragma once

#include <PCH.hpp>
#include <span>

namespace apollo {
	class App;

	struct AssetManagerSettings;

	enum class EAppResult : int8
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

		EAppResult (*m_OnInit)(const EntryPoint&, App&);

		AssetManagerSettings& m_AssetManagerSettings;
	};

	extern void GetEntryPoint(EntryPoint& out_entry);
} // namespace apollo