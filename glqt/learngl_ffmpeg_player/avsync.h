#ifndef AVSYNC_H
#define AVSYNC_H

#include <chrono>
#include <ctime>
#include <math.h>
#include <mutex>

using namespace std::chrono;

class AvSync
{
public:
    AvSync() {}

    void setClock(double pts)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        double time = getMicroseconds() / 1000000.0; // us -> s
        setClockAt(pts, time);
    }

    double getClock()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        double time = getMicroseconds() / 1000000.0;
        return m_ptsDrift + time;
    }

private:
    void setClockAt(double pts, double time)
    {
        m_pts = pts;
        m_ptsDrift = m_pts - time;
    }

    time_t getMicroseconds()
    {
        auto timeNow = steady_clock::now();
        auto duration = timeNow.time_since_epoch();

        return duration_cast<microseconds>(duration).count();
    }

private:
    double m_pts = 0.; // 存储当前时间戳
    double m_ptsDrift = 0.; // 存储当前时间戳与系统时间的差值
    std::mutex m_mutex;
};

#endif // AVSYNC_H
