#pragma once

#include <PCH.hpp>

#include "AssetFunctions.hpp"
#include "AssetRef.hpp"
#include <atomic>
#include <core/Queue.hpp>
#include <core/UniqueFunction.hpp>
#include <mutex>

struct SDL_GPUCommandBuffer;
struct SDL_GPUCopyPass;

namespace apollo::rdr {
	class GPUDevice;
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
		AssetLoader(rdr::GPUDevice& device)
			: m_Device(device)
		{}

		APOLLO_API void AddRequest(
			AssetRef<IAsset> asset,
			AssetImportFunc* importFunc,
			const AssetMetadata& metadata);

		APOLLO_API void ProcessRequests();

		static APOLLO_API SDL_GPUCommandBuffer* GetCurrentCommandBuffer() noexcept;
		static APOLLO_API SDL_GPUCopyPass* GetCurrentCopyPass() noexcept;

		template <class F>
		void RegisterCallback(F&& cbk)
		{
			std::unique_lock lock{m_CallbackMutex};
			m_LoadCallbacks.emplace_back(std::forward<F>(cbk));
		}

		~AssetLoader() = default;

	private:
		void DispatchCallbacks();
		void DoProcessRequests();

		rdr::GPUDevice& m_Device;
		Queue<AssetLoadRequest> m_Requests;
		std::atomic_uint32_t m_BatchSize = 0;
		std::vector<UniqueFunction<void()>> m_LoadCallbacks;
		std::mutex m_QueueMutex;
		std::mutex m_CallbackMutex;
	};
} // namespace apollo