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
		DEBUG_CHECK(m_Asset && m_Import && m_Metadata)
		{
			return false;
		}

		return m_Import(*m_Asset, *m_Metadata);
	}

	void AssetLoader::AddRequest(
		AssetRef<IAsset> asset,
		AssetImportFunc* loadFunc,
		const AssetMetadata& metadata)
	{
		m_Requests.AddEmplace(std::move(asset), loadFunc, &metadata);
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
			if (!request.m_Asset || request.m_Asset->GetState() != EAssetState::Loading)
				[[unlikely]]
				continue;
			BRK_LOG_TRACE(
				"Loading asset {}({})",
				request.m_Metadata->m_Name,
				request.m_Metadata->m_Id);
			[[maybe_unused]] const bool result = request();

			if (result)
			{
				request.m_Asset->SetState(EAssetState::Loaded);
				BRK_LOG_TRACE(
					"Asset {}({}) loaded successfully!",
					request.m_Metadata->m_Name,
					request.m_Metadata->m_Id);
			}
			else
			{
				request.m_Asset->SetState(EAssetState::Invalid);
				BRK_LOG_ERROR(
					"Asset {}({}) failed to load",
					request.m_Metadata->m_Name,
					request.m_Metadata->m_Id);
			}
		}

		SDL_EndGPUCopyPass(g_CopyPass);
		g_CopyPass = nullptr;
		SDL_SubmitGPUCommandBuffer(g_CommandBuffer);
		g_CommandBuffer = nullptr;
	}
} // namespace brk