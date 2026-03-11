#pragma once

#include <PCH.hpp>
#include <filesystem>
#include <span>

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

		EAppResult (*m_OnInit)(const EntryPoint&, App&) = nullptr;

		std::filesystem::path m_AssetRoot;
	};

	using AssetManagerInitFunc = IAssetManager&(
		const std::filesystem::path& settings,
		rdr::GPUDevice& gpuDevice,
		mt::ThreadPool& threadPool);

	extern EntryPoint GetEntryPoint(std::span<const char*> args);
} // namespace apollo