#include "AssetLoader.hpp"
#include <SDL3/SDL_gpu.h>
#include <asset/Asset.hpp>
#include <asset/AssetManager.hpp>
#include <core/App.hpp>
#include <core/Log.hpp>
#include <rendering/Device.hpp>

namespace {
	thread_local SDL_GPUCommandBuffer* g_CommandBuffer = nullptr;
	thread_local SDL_GPUCopyPass* g_CopyPass = nullptr;
} // namespace

namespace apollo {
	EAssetLoadResult AssetLoadRequest::operator()()
	{
		APOLLO_ASSERT(m_Asset, "Null assert in load request");
		APOLLO_ASSERT(m_Asset->GetState(), "Assset is in invalid state");

		if (!m_Import && m_Callback)
		{
			// Here we're not actually trying to load the asset:
			// we just want to invoke the callback
			if (m_Asset->IsLoading())
			{
				return EAssetLoadResult::TryAgain;
			}
			m_Callback(*m_Asset);
			return EAssetLoadResult::Success;
		}

		APOLLO_ASSERT(m_Metadata && m_Import, "Invalid asset load request");

		if (!m_Asset->IsLoading()) [[unlikely]]
		{
			APOLLO_LOG_TRACE(
				"Load Request for asset {}({}) was aborted",
				m_Metadata->m_Name,
				m_Metadata->m_Id);
			if (m_Callback)
				m_Callback(*m_Asset);

			return EAssetLoadResult::Aborted;
		}

#ifdef APOLLO_DEV
		if (!m_Asset->IsLoadingDeferred())
		{
			APOLLO_LOG_TRACE("Loading asset {}({})", m_Metadata->m_Name, m_Metadata->m_Id);
		}
#endif

		const EAssetLoadResult result = m_Import(*m_Asset, *m_Metadata);
		switch (result)
		{
		case EAssetLoadResult::Success:
			m_Asset->SetState(EAssetState::Loaded);
			APOLLO_LOG_TRACE(
				"Asset {}({}) loaded successfully!",
				m_Metadata->m_Name,
				m_Metadata->m_Id);
			break;
		case EAssetLoadResult::Failure:
			m_Asset->SetState(EAssetState::LoadingFailed);
			APOLLO_LOG_ERROR("Asset {}({}) failed to load", m_Metadata->m_Name, m_Metadata->m_Id);
			break;
		case EAssetLoadResult::TryAgain:
			m_Asset->SetState(EAssetState::Loading | EAssetState::LoadingDeferred);
			return result;
		default:
			APOLLO_LOG_CRITICAL("Invalid Asset Load Result {}", int32(result));
			return EAssetLoadResult::Failure;
		}

		if (m_Callback)
			m_Callback(*m_Asset);

		return result;
	}

	void AssetLoader::AddRequest(AssetLoadRequest req)
	{
		std::unique_lock lock{ m_Mutex };
		m_Requests.AddEmplace(std::move(req));
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
		if (m_RunningBatch || !m_Requests.GetSize())
			return;

		APOLLO_LOG_TRACE("Starting asset load batch");
		m_RunningBatch = true;
		m_ThreadPool.Enqueue(
			[this]()
			{
				DoProcessRequests();
			});
	}

	void AssetLoader::DoProcessRequests()
	{
		if (m_Device) [[likely]]
		{
			g_CommandBuffer = SDL_AcquireGPUCommandBuffer(m_Device.GetHandle());
			g_CopyPass = SDL_BeginGPUCopyPass(g_CommandBuffer);
		}

		for (;;)
		{
			std::unique_lock lock{ m_Mutex };
			if (!m_Requests.GetSize())
				break;

			AssetLoadRequest request = m_Requests.PopAndGetFront();
			lock.unlock();

			const EAssetLoadResult result = request();
			if (result == EAssetLoadResult::TryAgain)
			{
				m_Requests.AddEmplace(std::move(request));
			}
		}

		if (g_CopyPass) [[likely]]
		{
			SDL_EndGPUCopyPass(g_CopyPass);
			g_CopyPass = nullptr;
		}
		if (g_CommandBuffer) [[likely]]
		{
			SDL_SubmitGPUCommandBuffer(g_CommandBuffer);
			g_CommandBuffer = nullptr;
		}

		DispatchCallbacks();
		{
			std::unique_lock lock{ m_Mutex };
			m_RunningBatch = false;
		}
		m_Cond.notify_one();
	}

	void AssetLoader::WaitForCompletion()
	{
		std::unique_lock lock{ m_Mutex };
		while (m_RunningBatch)
		{
			m_Cond.wait(lock);
		}
	}

	void AssetLoader::Clear()
	{
		std::unique_lock lock{ m_Mutex };
		m_Requests.Clear();
	}

	void AssetLoader::DispatchCallbacks()
	{
		for (auto& cbk : m_LoadCallbacks)
		{
			cbk();
		}
	}
} // namespace apollo