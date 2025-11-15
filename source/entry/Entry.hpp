#pragma once

#include <PCH.hpp>
#include <span>
#include <filesystem>

namespace apollo::mt {
	class ThreadPool;
}

namespace apollo::rdr {
	class GPUDevice;
}

namespace apollo {
	class App;
	class IAssetManager;

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

		const std::span<const char*> m_Args;

		EAppResult (*m_OnInit)(const EntryPoint&, App&) = nullptr;

		std::filesystem::path m_AssetRoot;
		IAssetManager& (*m_InitAssetManager)(
			const std::filesystem::path& settings,
			rdr::GPUDevice& gpuDevice,
			mt::ThreadPool& threadPool) = nullptr;
	};

	extern void GetEntryPoint(EntryPoint& out_entry);
} // namespace apollo