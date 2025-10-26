#pragma once

#include <PCH.hpp>

#include "AssetFunctions.hpp"
#include "AssetRef.hpp"
#include <atomic>
#include <condition_variable>
#include <core/Queue.hpp>
#include <core/UniqueFunction.hpp>
#include <mutex>

struct SDL_GPUCommandBuffer;
struct SDL_GPUCopyPass;

namespace apollo::rdr {
	class GPUDevice;
}

namespace apollo::mt {
	class ThreadPool;
}

namespace apollo {
	class IAsset;

	struct AssetLoadRequest
	{
		AssetRef<IAsset> m_Asset;
		AssetImportFunc* m_Import = nullptr;
		const AssetMetadata* m_Metadata = nullptr;

		EAssetLoadResult operator()();
	};

	class AssetLoader
	{
	public:
		AssetLoader(rdr::GPUDevice& device, mt::ThreadPool& threadPool)
			: m_Device(device)
			, m_ThreadPool(threadPool)
		{}

		APOLLO_API void AddRequest(
			AssetRef<IAsset> asset,
			AssetImportFunc* importFunc,
			const AssetMetadata& metadata);

		APOLLO_API void ProcessRequests();
		APOLLO_API void WaitForCompletion();
		APOLLO_API void Clear();

		static APOLLO_API SDL_GPUCommandBuffer* GetCurrentCommandBuffer() noexcept;
		static APOLLO_API SDL_GPUCopyPass* GetCurrentCopyPass() noexcept;

		// This function is not thread-safe, and must be called from the main thread before any
		// asset load operations can begin. Ideally, you'd call this during the post-init phase at
		// the start of the app
		template <class F>
		void RegisterCallback(F&& cbk)
		{
			m_LoadCallbacks.emplace_back(std::forward<F>(cbk));
		}

		~AssetLoader() { WaitForCompletion(); }

	private:
		void DispatchCallbacks();
		void DoProcessRequests();

		rdr::GPUDevice& m_Device;
		mt::ThreadPool& m_ThreadPool;
		std::condition_variable m_Cond;
		Queue<AssetLoadRequest> m_Requests;
		std::atomic_bool m_RunningBatch = false;
		std::vector<UniqueFunction<void()>> m_LoadCallbacks;
		std::mutex m_Mutex;
	};
} // namespace apollo