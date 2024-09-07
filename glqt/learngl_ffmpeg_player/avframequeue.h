#ifndef AVFRAMEQUEUE_H
#define AVFRAMEQUEUE_H

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
    #include <libavutil/channel_layout.h>
}


class AvFrameQueue
{
public:
    AvFrameQueue();
    ~AvFrameQueue();

    void abort();

    size_t size();

    void push(AVFrame* frame = nullptr);
    AVFrame* pop(const int timeout);
    AVFrame* front();

private:
     void release();

private:
    MultiMediaQueue<AVFrame*> m_frameQue;
};

#endif // AVFRAMEQUEUE_H
