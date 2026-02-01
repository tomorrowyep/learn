#ifndef __KTHREADPOOL_H__
#define __KTHREADPOOL_H__

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

namespace kEngine
{

class KThreadPool
{
public:
	KThreadPool(uint32_t threadCount = 0)
	{
		if (threadCount == 0)
		{
			threadCount = std::max(1u, std::thread::hardware_concurrency() - 1);
		}

		for (uint32_t i = 0; i < threadCount; ++i)
		{
			m_workers.emplace_back([this]()
			{
				while (true)
				{
					std::function<void()> task;

					{
						std::unique_lock<std::mutex> lock(m_mutex);
						m_condition.wait(lock, [this]()
						{
							return m_stop || !m_tasks.empty();
						});

						if (m_stop && m_tasks.empty())
							return;

						task = std::move(m_tasks.front());
						m_tasks.pop();
					}

					task();

					{
						std::lock_guard<std::mutex> lock(m_mutex);
						--m_activeTasks;
					}
					m_finishCondition.notify_all();
				}
			});
		}
	}

	~KThreadPool()
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_stop = true;
		}

		m_condition.notify_all();

		for (auto& worker : m_workers)
		{
			if (worker.joinable())
				worker.join();
		}
	}

	KThreadPool(const KThreadPool&) = delete;
	KThreadPool& operator=(const KThreadPool&) = delete;

	template<typename F, typename... Args>
	auto Submit(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
	{
		using ReturnType = typename std::result_of<F(Args...)>::type;

		auto task = std::make_shared<std::packaged_task<ReturnType()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);

		std::future<ReturnType> result = task->get_future();

		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_stop)
				throw std::runtime_error("Submit on stopped KThreadPool");

			++m_activeTasks;
			m_tasks.emplace([task]() { (*task)(); });
		}

		m_condition.notify_one();
		return result;
	}

	void WaitIdle()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_finishCondition.wait(lock, [this]()
		{
			return m_tasks.empty() && m_activeTasks == 0;
		});
	}

	uint32_t GetThreadCount() const
	{
		return static_cast<uint32_t>(m_workers.size());
	}

private:
	std::vector<std::thread>          m_workers;
	std::queue<std::function<void()>> m_tasks;

	std::mutex                        m_mutex;
	std::condition_variable           m_condition;
	std::condition_variable           m_finishCondition;

	bool                              m_stop = false;
	uint32_t                          m_activeTasks = 0;
};

} // namespace kEngine

#endif // !__KTHREADPOOL_H__
