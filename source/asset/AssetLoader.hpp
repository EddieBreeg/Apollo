#pragma once

#include <PCH.hpp>

#include "AssetFunctions.hpp"
#include "AssetRef.hpp"
#include <core/Queue.hpp>
#include <core/UniqueFunction.hpp>

struct SDL_GPUCommandBuffer;
struct SDL_GPUCopyPass;

namespace brk::rdr {
	class GPUDevice;
}

namespace brk {
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

		BRK_API void AddRequest(
			AssetRef<IAsset> asset,
			AssetImportFunc* importFunc,
			const AssetMetadata& metadata);

		BRK_API void ProcessRequests();

		static BRK_API SDL_GPUCommandBuffer* GetCurrentCommandBuffer() noexcept;
		static BRK_API SDL_GPUCopyPass* GetCurrentCopyPass() noexcept;

		template <class F>
		void RegisterCallback(F&& cbk)
		{
			m_LoadCallbacks.emplace_back(std::forward<F>(cbk));
		}

		~AssetLoader() = default;

	private:
		void DispatchCallbacks();

		rdr::GPUDevice& m_Device;
		Queue<AssetLoadRequest> m_Requests;
		std::vector<UniqueFunction<void()>> m_LoadCallbacks;
	};
} // namespace brk