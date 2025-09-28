#pragma once

#include <PCH.hpp>

#include "Importer.hpp"
#include "AssetRef.hpp"
#include <core/Queue.hpp>

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

	class BRK_API AssetLoader
	{
	public:
		AssetLoader(rdr::GPUDevice& device)
			: m_Device(device)
		{}

		void AddRequest(AssetRef<IAsset> asset, AssetImportFunc* importFunc, const AssetMetadata& metadata);

		void ProcessRequests();

		static SDL_GPUCommandBuffer* GetCurrentCommandBuffer() noexcept;
		static SDL_GPUCopyPass* GetCurrentCopyPass() noexcept;

		~AssetLoader() = default;

	private:
		rdr::GPUDevice& m_Device;
		Queue<AssetLoadRequest> m_Requests;
	};
} // namespace brk