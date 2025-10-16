#include "AssetLoader.hpp"
#include <SDL3/SDL_gpu.h>
#include <asset/Asset.hpp>
#include <asset/AssetManager.hpp>
#include <core/App.hpp>
#include <core/Log.hpp>
#include <core/ULIDFormatter.hpp>
#include <rendering/Device.hpp>

namespace {
	thread_local SDL_GPUCommandBuffer* g_CommandBuffer = nullptr;
	thread_local SDL_GPUCopyPass* g_CopyPass = nullptr;
} // namespace

namespace apollo {
	EAssetLoadResult AssetLoadRequest::operator()()
	{
		DEBUG_CHECK(m_Asset && m_Import && m_Metadata)
		{
			return EAssetLoadResult::Failure;
		}

		return m_Import(*m_Asset, *m_Metadata);
	}

	void AssetLoader::AddRequest(
		AssetRef<IAsset> asset,
		AssetImportFunc* loadFunc,
		const AssetMetadata& metadata)
	{
		std::unique_lock lock{ m_QueueMutex };
		m_Requests.AddEmplace(std::move(asset), loadFunc, &metadata);
		// if we are currently processing assets, then add the new request to the current batch
		if (m_BatchSize)
			++m_BatchSize;
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
		// if batch size is non-zero, a thread is already processing the assets, we don't need to do
		// anything else
		if (!m_Requests.GetSize() || m_BatchSize)
			return;

		m_BatchSize = m_Requests.GetSize();
		mt::ThreadPool& tp = App::GetInstance()->GetThreadPool();
		tp.Enqueue(
			[this]()
			{
				DoProcessRequests();
			});
	}

	void AssetLoader::DoProcessRequests()
	{
		g_CommandBuffer = SDL_AcquireGPUCommandBuffer(m_Device.GetHandle());
		g_CopyPass = SDL_BeginGPUCopyPass(g_CommandBuffer);

		while (m_BatchSize)
		{
			std::unique_lock lock{ m_QueueMutex };
			AssetLoadRequest request = m_Requests.PopAndGetFront();
			lock.unlock();

			if (!request.m_Asset || request.m_Asset->GetState() != EAssetState::Loading)
				[[unlikely]]
				continue;

			APOLLO_LOG_TRACE(
				"Loading asset {}({})",
				request.m_Metadata->m_Name,
				request.m_Metadata->m_Id);
			const EAssetLoadResult result = request();

			switch (result)
			{
			case apollo::EAssetLoadResult::Success:
				request.m_Asset->SetState(EAssetState::Loaded);
				APOLLO_LOG_TRACE(
					"Asset {}({}) loaded successfully!",
					request.m_Metadata->m_Name,
					request.m_Metadata->m_Id);
				--m_BatchSize;
				continue;
			case apollo::EAssetLoadResult::TryAgain:
				m_Requests.AddEmplace(std::move(request));
				continue;
			default:
				request.m_Asset->SetState(EAssetState::Invalid);
				APOLLO_LOG_ERROR(
					"Asset {}({}) failed to load",
					request.m_Metadata->m_Name,
					request.m_Metadata->m_Id);
				--m_BatchSize;
			}
		}

		SDL_EndGPUCopyPass(g_CopyPass);
		g_CopyPass = nullptr;
		SDL_SubmitGPUCommandBuffer(g_CommandBuffer);
		g_CommandBuffer = nullptr;

		DispatchCallbacks();
	}

	void AssetLoader::DispatchCallbacks()
	{
		std::unique_lock lock{ m_CallbackMutex };
		for (auto& cbk : m_LoadCallbacks)
		{
			cbk();
		}
	}
} // namespace apollo