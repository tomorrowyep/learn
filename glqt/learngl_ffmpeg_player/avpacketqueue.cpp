#include "avpacketqueue.h"

AvPacketQueue::AvPacketQueue()
{

}

AvPacketQueue::~AvPacketQueue()
{
    /*auto data = m_avPacketQue.data();
    if (!data.empty())
    {
        while(data.size())
        {
            AVPacket* packet = data.front();
            data.pop();
            if (packet)
                av_packet_free(&packet);
        }
    }*/
}

void AvPacketQueue::abort()
{
    release();
    m_avPacketQue.abort();
}

size_t AvPacketQueue::size()
{
    return m_avPacketQue.size();
}

void AvPacketQueue::release()
{
    while(true)
    {
        AVPacket* packet = nullptr;
        m_avPacketQue.pop(packet, 1);
        if (packet)
        {
            av_packet_free(&packet);
            continue;
        }
        else
        {
            break;
        }
    }
}

void AvPacketQueue::push(AVPacket *packet)
{
    if (!packet)
        return;

    /* AVPacket* newPacket = av_packet_clone(packet) 会触发数据的复制，降低性能*/
    // 这种方式类似移动构造，只是转移（避免了复制），然后将原指针置空
    AVPacket* newPacket = av_packet_alloc();
    av_packet_move_ref(newPacket, packet);
    m_avPacketQue.push(newPacket);
}

AVPacket *AvPacketQueue::pop(const int timeout)
{
    AVPacket* packet = nullptr;
    m_avPacketQue.pop(packet, timeout);

    return packet;
}
