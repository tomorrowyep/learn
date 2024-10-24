#ifndef MULTIMEDIAQUEUE_H
#define MULTIMEDIAQUEUE_H

#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>

template <typename T>
class MultiMediaQueue
{
public:
    MultiMediaQueue() {}
    ~MultiMediaQueue() {}

    void abort()
    {
        m_abort = true;
        m_condition.notify_all();
    }

    void push(T val)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_abort)
            return;

        m_queue.push(val);
        m_condition.notify_one();
    }

    void pop(T &val, const int timeout = 0)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_queue.empty())
        {
            // 等待push或者是超时唤醒（ms）
            m_condition.wait_for(lock, std::chrono::milliseconds(timeout), [this]
            {
                return !m_queue.empty() || m_abort;
            });
        }

        if (m_queue.empty() || m_abort)
            return;

        val = m_queue.front();
        m_queue.pop();
    }

    void front(T &val)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_queue.empty() || m_abort)
            return;

        val = m_queue.front();
    }

    size_t size()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    std::queue<T>& data()
    {
        return m_queue;
    }

private:
    bool m_abort = false;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::queue<T> m_queue;
};

#endif // MULTIMEDIAQUEUE_H
