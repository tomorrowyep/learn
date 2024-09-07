#ifndef VIDEOCODECTHREAD_H
#define VIDEOCODECTHREAD_H

#include "multimediathread.h"
#include "avpacketqueue.h"
#include "avframequeue.h"

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

class DecodeThread : public MultipleMediaThread
{
public:
    DecodeThread(AvPacketQueue* pPacketQue, AvFrameQueue* pFrameQue);
    ~DecodeThread();

    bool init(AVCodecParameters* paras);
    int start();
    int stop();

    virtual void run() override;

private:
    AVCodecContext* m_codeCtx = nullptr;

    AvPacketQueue* m_pPacketQue = nullptr;
    AvFrameQueue* m_frameQue = nullptr;
};

#endif // VIDEOCODECTHREAD_H
