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
		DEBUG_CHECK(m_Asset && m_Import && m_Metadata)
		{
			return EAssetLoadResult::Failure;
		}

		return m_Import(*m_Asset, *m_Metadata);
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

			if (!request.m_Asset || !request.m_Asset->IsLoading()) [[unlikely]]
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
				break;
			case apollo::EAssetLoadResult::TryAgain:
				request.m_Asset->SetState(EAssetState::Loading | EAssetState::LoadingDeferred);
				m_Requests.AddEmplace(std::move(request));
				break;
			default:
				request.m_Asset->SetState(EAssetState::Invalid);
				APOLLO_LOG_ERROR(
					"Asset {}({}) failed to load",
					request.m_Metadata->m_Name,
					request.m_Metadata->m_Id);
				break;
			}
			if (request.m_Callback)
				request.m_Callback(*request.m_Asset);
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