#pragma once

/** \file AssetLoader.hpp */

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
	 * \brief Coroutine type used to load assets asynchronously
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
	 * \brief Request object, submitted to the \ref apollo::AssetLoader "asset loader"
	 */
	struct AssetLoadRequest
	{
		AssetRef<IAsset> m_Asset; /*!< The asset to load */
		AssetLoadTask m_Task;	  /*!< The coroutine to invoke */
		const AssetMetadata* m_Metadata = nullptr;
		UniqueFunction<void(IAsset&)> m_Callback; /*!< If not null, this callback gets invoked once
													 the asset completes loading*/

		/// Invokes the load task
		EAssetLoadResult operator()();
	};

	/**
	 * \brief This class is in charge of processing all asset load requests from the \ref
	 * IAssetManager "asset manager"
	 * \details Assets are loaded on a separate thread using mt::ThreadPool.
	 */
	class AssetLoader
	{
	public:
		/**
		 * \brief Initiazes the loader. This is called by the main App class.
		 */
		AssetLoader(rdr::GPUDevice& device, mt::ThreadPool& threadPool)
			: m_Device(device)
			, m_ThreadPool(threadPool)
		{}

		/**
		 * \brief Adds a new request to the queue. This is typically called from the \ref
		 * IAssetManager "asset manager".
		 */
		APOLLO_API void AddRequest(AssetLoadRequest request);

		/**
		 * \brief Called every frame to process the queue
		 */
		APOLLO_API void ProcessRequests();
		/**
		 * \brief Waits for the current asset batch to finish.
		 * \details This function blocks until the worker thread is done processing all pending
		 * requests, e.g. after loading a scene.
		 */
		APOLLO_API void WaitForCompletion();
		/**
		 * \brief Clears the queue
		 */
		APOLLO_API void Clear();

		static APOLLO_API SDL_GPUCommandBuffer* GetCurrentCommandBuffer() noexcept;
		static APOLLO_API SDL_GPUCopyPass* GetCurrentCopyPass() noexcept;

		/**
		 * \brief Registers a callback which will be called after all assets in a batch have been
		 * processed
		 * \note This function is not thread-safe, and must be called from the main thread before
		 * any asset load operations can begin. Ideally, you'd call this during the post-init phase
		 * of a system right before the game actually starts
		 */
		template <class F>
		void RegisterCallback(F&& cbk)
		{
			m_LoadCallbacks.emplace_back(std::forward<F>(cbk));
		}

		/**
		 * \brief Calls AssetLoader::WaitForCompletion()
		 */
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
	 * \brief Asset coroutine promise type, used internally by AssetTask
	 */
	class AssetPromise : public coro::NoopPromise<>
	{
	public:
		using HandleType = std::coroutine_handle<AssetPromise>;

	public:
		AssetPromise() = default;
		AssetLoadTask get_return_object() noexcept
		{
			return AssetLoadTask{ std::coroutine_handle<AssetPromise>::from_promise(*this) };
		}

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
} // namespace apollo