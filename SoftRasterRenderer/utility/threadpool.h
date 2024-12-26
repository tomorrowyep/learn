#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include <atomic>
#include <thread>
#include <vector>
#include <queue>
#include <condition_variable>
#include <future>
#include <mutex>
#include <functional>
#include <memory>

// TODO:增加任务的优先级以及支持取消任务
class ThreadPool
{
public:
	using Task = std::function<void()>;

	ThreadPool(const ThreadPool& other) = delete;
	ThreadPool& operator= (const ThreadPool& other) = delete;

	~ThreadPool()
	{
		_stop();
	}

	static ThreadPool& instance()
	{
		static ThreadPool ins;
		return ins;
	}

	int idleThreadCount()
	{
		return m_threadNum;
	}

	template<typename Fn, typename... Args>
	decltype(auto) commit(Fn&& func, Args&&... args)
	{
		using retType = std::invoke_result_t<Fn, Args...>;//decltype(func(args...))

		if (m_bIsStop.load())
			return std::future<retType>();

		auto task = std::make_shared<std::packaged_task<retType()>>(std::bind(std::forward<Fn>(func), std::forward<Args>(args)...));
		std::future<retType> ret = task->get_future();
		{
			std::lock_guard<std::mutex> locker(m_mutex);
			m_taskQue.emplace([task]() { (*task)(); });
		}

		m_cond.notify_one();
		return ret;
	}

private:
	ThreadPool() : m_threadNum(1), m_bIsStop(false)
	{
		size_t coreCount = std::thread::hardware_concurrency();
		if (coreCount != 0)
			m_threadNum.store(coreCount);

		_start();
	}

	void _start()
	{
		for (int i = 0; i < m_threadNum.load(); ++i)
		{
			m_threads.emplace_back([this]()
				{
					while (!m_bIsStop.load())
					{
						Task task;
						{
							std::unique_lock<std::mutex> locker(m_mutex);
							if (m_taskQue.empty())
								m_cond.wait(locker, [this] { return !m_taskQue.empty() || m_bIsStop.load(); });

							// 当停止时又没有任务就可以退出了
							if (m_taskQue.empty())
								return;

							task = std::move(m_taskQue.front());
							m_taskQue.pop();
						}

						m_threadNum--;
						task();
						m_threadNum++;
					}
				});
		}
	}

	void _stop()
	{
		m_bIsStop.store(true);
		m_cond.notify_all();

		for (auto& thread : m_threads)
		{
			if (thread.joinable())
				thread.join();
		}
	}

private:
	std::atomic_int m_threadNum;
	std::atomic_bool m_bIsStop;
	std::mutex m_mutex;
	std::condition_variable m_cond;
	std::queue<Task> m_taskQue;
	std::vector<std::thread> m_threads;
};
#endif // !__THREADPOOL_H__
