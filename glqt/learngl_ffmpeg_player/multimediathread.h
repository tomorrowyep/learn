#ifndef MULTIPLEMEDIATHREAD_H
#define MULTIPLEMEDIATHREAD_H

#include <thread>

class MultipleMediaThread
{
public:
    MultipleMediaThread() {}
    ~MultipleMediaThread()
    {
        if (m_thread)
            stop();
    }

    void start()
    {

    }
    void stop()
    {
       m_abort = true;

       if (m_thread)
       {
           if (m_thread->joinable())
               m_thread->join();

           delete m_thread;
           m_thread = nullptr;
       }
    }

    virtual void run() = 0;

public:
    bool m_abort = false;
    std::thread *m_thread = nullptr;
};

#endif // MULTIPLEMEDIATHREAD_H
