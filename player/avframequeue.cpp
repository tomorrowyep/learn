#include "avframequeue.h"

AvFrameQueue::AvFrameQueue()
{

}

AvFrameQueue::~AvFrameQueue()
{
    /*auto data = m_frameQue.data();
    if (!data.empty())
    {
        while(data.size())
        {
            AVFrame* frame = data.front();
            data.pop();
            if (frame)
                av_frame_free(&frame);
        }
    }*/
}

void AvFrameQueue::abort()
{
    release();
    m_frameQue.abort();
}

size_t AvFrameQueue::size()
{
     return m_frameQue.size();
}

void AvFrameQueue::push(AVFrame *frame)
{
    if (!frame)
        return;

    AVFrame* newFrame = av_frame_alloc();
    av_frame_move_ref(newFrame, frame);
    m_frameQue.push(newFrame);
}

AVFrame *AvFrameQueue::pop(const int timeout)
{
    AVFrame* frame = nullptr;
    m_frameQue.pop(frame, timeout);

    return frame;
}

AVFrame *AvFrameQueue::front()
{
    AVFrame* frame = nullptr;
    m_frameQue.front(frame);

    return frame;
}

void AvFrameQueue::release()
{
    while(true)
    {
        AVFrame* frame = nullptr;
        m_frameQue.pop(frame, 1);
        if (frame)
        {
            av_frame_free(&frame);
            continue;
        }
        else
        {
            break;
        }
    }
}
