#pragma once

#include <PCH.hpp>

#include "Queue.hpp"
#include "UniqueFunction.hpp"
#include <condition_variable>
#include <exception>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

namespace brk::mt {
	class ThreadPool
	{
	public:
		static inline const uint32 DefaultThreadCount = Max(1u, std::thread::hardware_concurrency());

		inline explicit ThreadPool(uint32 threadCount = DefaultThreadCount);
		inline ~ThreadPool();

		ThreadPool(ThreadPool&&) = delete;
		ThreadPool& operator=(ThreadPool&&) = delete;

		/*
		 * Adds a simple job to the queue. This is the preferred overload, as it does not require
		 * wrapping the functor in a lambda.
		 * \remark The job is not guaranteed to get executed:
		 * Stop might get called before a thread gets to it
		 */
		template <class F>
		void Enqueue(F&& func) requires(std::is_invocable_r_v<void, F>);

		/*
		 * Adds a simple job with arguments to the queue. This overload wraps the functor and its
		 * arguments in a lambda. Consider using the overload above instead.
		 * \remark This job is not guaranteed to get executed:
		 * Stop might get called before a thread gets to it
		 */
		template <class F, class... Args>
		void Enqueue(F&& func, Args&&... args) requires(std::is_invocable_r_v<void, F, Args...>);

		/*
		 * Adds a simple job with arguments to the queue.
		 * \remark This job is not guaranteed to get executed:
		 * Stop might get called before a thread gets to it
		 */
		template <class F, class... Args>
		[[nodiscard]] auto EnqueueAndGetFuture(F&& func, Args&&... args)
			-> std::future<decltype(func(std::move(args)...))> requires(
				std::is_invocable_v<F, Args...>);

		[[nodiscard]] uint32 GetThreadCount() const noexcept { return (uint32)m_Threads.size(); }

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
				return;

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

} // namespace brk::mt