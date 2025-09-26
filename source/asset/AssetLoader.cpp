#include "AssetLoader.hpp"
#include <SDL3/SDL_gpu.h>
#include <asset/Asset.hpp>
#include <asset/AssetManager.hpp>
#include <core/Log.hpp>
#include <core/ULIDFormatter.hpp>
#include <rendering/Device.hpp>

namespace {
	thread_local SDL_GPUCommandBuffer* g_CommandBuffer = nullptr;
	thread_local SDL_GPUCopyPass* g_CopyPass = nullptr;
} // namespace

namespace brk {
	bool AssetLoadRequest::operator()()
	{
		DEBUG_CHECK(m_Asset && m_Import)
		{
			return false;
		}

		return m_Import(*m_Asset);
	}

	void AssetLoader::AddRequest(std::shared_ptr<IAsset> asset, AssetImportFunc* loadFunc)
	{
		m_Requests.AddEmplace(asset, loadFunc);
	}

	SDL_GPUCommandBuffer* AssetLoader::GetCurrentCommandBuffer() noexcept
	{
		return g_CommandBuffer;
	}

	SDL_GPUCopyPass* AssetLoader::GetCurrentCopyPass() noexcept
	{
		return g_CopyPass;
	}

	void AssetLoader::ProcessRequests()
	{
		if (!m_Requests.GetSize())
			return;

		// TODO: Move all of this into separate threads

		g_CommandBuffer = SDL_AcquireGPUCommandBuffer(m_Device.GetHandle());
		g_CopyPass = SDL_BeginGPUCopyPass(g_CommandBuffer);

		while (m_Requests.GetSize())
		{
			AssetLoadRequest request = m_Requests.PopAndGetFront();
			if (!request.m_Asset) [[unlikely]]
				continue;
#ifdef BRK_DEV
			const auto& metadata = request.m_Asset->GetMetadata();
			BRK_LOG_TRACE("Loading asset {}({})", metadata.m_Name, metadata.m_Id);
#endif
			[[maybe_unused]] const bool result = request();

#ifdef BRK_DEV
			if (result)
			{
				BRK_LOG_TRACE("Asset {}({}) loaded successfully!", metadata.m_Name, metadata.m_Id);
			}
			else
			{
				BRK_LOG_ERROR("Asset {}({}) failed to load", metadata.m_Name, metadata.m_Id);
			}
#endif
		}

		SDL_EndGPUCopyPass(g_CopyPass);
		g_CopyPass = nullptr;
		SDL_SubmitGPUCommandBuffer(g_CommandBuffer);
		g_CommandBuffer = nullptr;
	}
} // namespace brk