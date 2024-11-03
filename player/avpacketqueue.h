#ifndef PACKETQUEUE_H
#define PACKETQUEUE_H

#include "multimediaqueue.h"

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/opt.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/samplefmt.h>
    #include <libavutil/display.h>
    #include <libswscale/swscale.h>
    #include <libavdevice/avdevice.h>
    #include <libswresample/swresample.h>
}

class AvPacketQueue
{
public:
    AvPacketQueue();
    ~AvPacketQueue();

    void abort();

    size_t size();

    void push(AVPacket *packet = nullptr);
    AVPacket* pop(const int timeout);

    void release();

private:
    MultiMediaQueue<AVPacket*> m_avPacketQue;
};

#endif // PACKETQUEUE_H
