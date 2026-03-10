#pragma once

#include <PCH.hpp>

#include "Asset.hpp"
#include "AssetFunctions.hpp"
#include "AssetRef.hpp"
#include <atomic>
#include <condition_variable>
#include <core/Coroutine.hpp>
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

	class AssetPromise;

	/**
	 * Coroutine type used to load assets asynchronously
	 */
	struct AssetLoadTask : public coro::Coroutine<AssetPromise>
	{
		using Base = coro::Coroutine<AssetPromise>;
		AssetLoadTask() = default;
		AssetLoadTask(HandleType handle)
			: Base::Coroutine(handle)
		{}
		APOLLO_API EAssetLoadResult operator()();
	};

	/**
	 * Request object, submitted to the asset loader
	 */
	struct AssetLoadRequest
	{
		AssetRef<IAsset> m_Asset;
		AssetLoadTask m_Task;
		const AssetMetadata* m_Metadata = nullptr;
		UniqueFunction<void(IAsset&)> m_Callback;

		EAssetLoadResult operator()();
	};

	class AssetLoader
	{
	public:
		AssetLoader(rdr::GPUDevice& device, mt::ThreadPool& threadPool)
			: m_Device(device)
			, m_ThreadPool(threadPool)
		{}

		APOLLO_API void AddRequest(AssetLoadRequest request);

		APOLLO_API void ProcessRequests();
		APOLLO_API void WaitForCompletion();
		APOLLO_API void Clear();

		static APOLLO_API SDL_GPUCommandBuffer* GetCurrentCommandBuffer() noexcept;
		static APOLLO_API SDL_GPUCopyPass* GetCurrentCopyPass() noexcept;

		/* \brief Registers a callback which will be called after all assets in a batch have been
		 * processed
		 * \note This function is not thread-safe, and must be called from the main thread before
		 * any asset load operations can begin. Ideally, you'd call this during the post-init phase
		 * at the start of the app
		 */
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

	/**
	 * Asset coroutine promise type, used internally
	 */
	class AssetPromise : public coro::NoopPromise<>
	{
	public:
		using HandleType = std::coroutine_handle<AssetPromise>;

	public:
		AssetPromise() = default;
		AssetLoadTask get_return_object() noexcept;

		void return_value(bool success) noexcept
		{
			m_Result = success ? EAssetLoadResult::Success : EAssetLoadResult::Failure;
		}
		[[nodiscard]] bool CanResume() const noexcept
		{
			return !(m_Awaiting && m_Awaiting->IsLoading());
		}
		[[nodiscard]] EAssetLoadResult GetResult() const noexcept { return m_Result; }

		bool await_ready() const noexcept { return m_Result != EAssetLoadResult::TryAgain; }

	private:
		template <Asset A>
		struct AssetAwaiter
		{
			bool await_ready() const noexcept { return !(m_Asset && m_Asset->IsLoading()); }
			void await_suspend(HandleType handle) noexcept
			{
				handle.promise().m_Result = EAssetLoadResult::TryAgain;
			}
			AssetRef<A> await_resume() noexcept { return std::move(m_Asset); }

			AssetRef<A> m_Asset;
		};

	public:
		template <class A>
		AssetAwaiter<A> await_transform(AssetRef<A> ref)
		{
			m_Awaiting = StaticPointerCast<IAsset>(ref);
			return { ref };
		}

	private:
		EAssetLoadResult m_Result = EAssetLoadResult::TryAgain;
		AssetRef<IAsset> m_Awaiting; // used if we are waiting on an another asset
	};

	inline AssetLoadTask AssetPromise::get_return_object() noexcept
	{
		return AssetLoadTask{ std::coroutine_handle<AssetPromise>::from_promise(*this) };
	}
} // namespace apollo