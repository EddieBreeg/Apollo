#pragma once

#include <PCH.hpp>

#include "NumConv.hpp"
#include "Queue.hpp"
#include "UniqueFunction.hpp"
#include <condition_variable>
#include <exception>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

/** \file ThreadPool.hpp */

/**
 * \namespace apollo::mt
 * \brief Multithreading utilities
 */
namespace apollo::mt {
	/** \brief Simple thread pool implementation

	This gets used internally in places where concurrency is desired, for example the AssetLoader.
	Jobs are stored using a Queue container.
	 */
	class ThreadPool
	{
	public:
		static inline const uint32 DefaultThreadCount = Max(1u, std::thread::hardware_concurrency());

		/**
		 * \brief Creates and immediately kicks off the threads.
		 */
		inline explicit ThreadPool(uint32 threadCount = DefaultThreadCount);
		inline ~ThreadPool();

		ThreadPool(ThreadPool&&) = delete;
		ThreadPool& operator=(ThreadPool&&) = delete;

		[[nodiscard]] bool IsRunning() const noexcept { return m_Running; }

		/** \name Enqueue
		 * \brief Adds a job add the end of the queue
		 * \note A job is not guaranteed to get executed: \ref ThreadPool::Stop "Stop()"
		 * might get called before it gets the chance.
		 * @{ */

		/**
		 * \brief This is the preferred overload, as it does not require
		 * wrapping the functor in a lambda.
		 */
		template <class F>
		inline void Enqueue(F&& func) requires(std::is_invocable_r_v<void, F>);

		/**
		 * \brief This overload wraps the functor and its
		 * arguments in a lambda. Consider using the overload above instead.
		 */
		template <class F, class... Args>
		inline void Enqueue(F&& func, Args&&... args) requires(std::is_invocable_r_v<void, F, Args...>);

		/** @} */
		
		/**
		 * \brief Adds a job to the queue and returns an associated future
		 * \retval std::future A future object through which the result will be made a available if the
		 * job completes.
		 */
		template <class F, class... Args>
		[[nodiscard]] inline auto EnqueueAndGetFuture(F&& func, Args&&... args)
			-> std::future<decltype(func(std::move(args)...))> requires(
				std::is_invocable_v<F, Args...>);

		[[nodiscard]] uint32 GetThreadCount() const noexcept
		{
			return NumCast<uint32>(m_Threads.size());
		}

		/** \brief Marks the pool as stopped and notifies all threads
		 * \note This function does not block, but jobs might still be running by the time it
		 * returns.
		 */
		inline void Stop();

	private:
		inline void Loop();

		std::vector<std::thread> m_Threads;
		std::mutex m_Mutex;
		std::condition_variable m_Cv;
		Queue<UniqueFunction<void()>> m_Jobs;

		bool m_Running = true;
	};

	ThreadPool::ThreadPool(uint32 n)
	{
		m_Threads.reserve(n);
		while (n--)
		{
			m_Threads.emplace_back(&ThreadPool::Loop, this);
		}
	}

	ThreadPool::~ThreadPool()
	{
		Stop();
		for (std::thread& th : m_Threads)
			th.join();
	}

	void ThreadPool::Loop()
	{
		for (;;)
		{
			std::unique_lock lock{ m_Mutex };
			while (m_Running && !m_Jobs.GetSize())
			{
				m_Cv.wait(lock);
			}
			if (!m_Running)
			{
				return;
			}

			UniqueFunction job = m_Jobs.PopAndGetFront();
			lock.unlock();

			job();
		}
	}

	void ThreadPool::Stop()
	{
		{
			std::unique_lock lock{ m_Mutex };
			m_Running = false;
		}
		m_Cv.notify_all();
	}

	template <class F>
	void ThreadPool::Enqueue(F&& func) requires(std::is_invocable_r_v<void, F>)
	{
		{
			std::unique_lock lock{ m_Mutex };
			m_Jobs.AddEmplace(std::forward<F>(func));
		}
		m_Cv.notify_one();
	}

	template <class F, class... Args>
	void ThreadPool::Enqueue(F&& func, Args&&... args)
		requires(std::is_invocable_r_v<void, F, Args...>)
	{
		static_assert(
			((!std::is_lvalue_reference_v<decltype(args)>) && ...),
			"Don't pass arguments by reference, use std::ref or lambda captures or whatever");
		{
			std::unique_lock lock{ m_Mutex };
			m_Jobs.AddEmplace(
				[func = std::forward<F>(func), ... args = std::move(args)]() mutable
				{
					func(std::move(args)...);
				});
		}
		m_Cv.notify_one();
	}

	template <class F, class... Args>
	auto ThreadPool::EnqueueAndGetFuture(F&& func, Args&&... args)
		-> std::future<decltype(func(std::move(args)...))> requires(std::is_invocable_v<F, Args...>)
	{
		static_assert(
			((!std::is_lvalue_reference_v<decltype(args)>) && ...),
			"Don't pass arguments by reference, use std::ref or lambda captures or whatever");

		using R = decltype(func(std::move(args)...));
		std::promise<R> promise;
		std::future future = promise.get_future();
		std::unique_lock lock{ m_Mutex };
		m_Jobs.AddEmplace(
			[func = std::forward<F>(func),
			 ... args = std::move(args),
			 promise = std::move(promise)]() mutable
			{
				try
				{
					if constexpr (std::is_void_v<R>)
					{
						func(std::move(args)...);
						promise.set_value();
					}
					else
					{
						promise.set_value(func(std::move(args)...));
					}
				}
				catch (...)
				{
					promise.set_exception(std::current_exception());
				}
			});
		lock.unlock();

		m_Cv.notify_one();
		return future;
	}

} // namespace apollo::mt