#pragma once

#include <PCH.hpp>

#include "Importer.hpp"
#include <core/Queue.hpp>
#include <memory>

struct SDL_GPUCommandBuffer;
struct SDL_GPUCopyPass;

namespace brk::rdr {
	class GPUDevice;
}

namespace brk {
	class IAsset;

	struct AssetLoadRequest
	{
		std::shared_ptr<IAsset> m_Asset;
		AssetImportFunc* m_Import = nullptr;

		bool operator()();
	};

	class BRK_API AssetLoader
	{
	public:
		AssetLoader(rdr::GPUDevice& device)
			: m_Device(device)
		{}

		void AddRequest(std::shared_ptr<IAsset> asset, AssetImportFunc* importFunc);

		void ProcessRequests();

		static SDL_GPUCommandBuffer* GetCurrentCommandBuffer() noexcept;
		static SDL_GPUCopyPass* GetCurrentCopyPass() noexcept;

		~AssetLoader() = default;

	private:
		rdr::GPUDevice& m_Device;
		Queue<AssetLoadRequest> m_Requests;
	};
} // namespace brk