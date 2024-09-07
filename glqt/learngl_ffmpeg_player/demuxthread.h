#ifndef __DEMUXTHREAD__
#define __DEMUXTHREAD__

#include <string>

#include "multimediathread.h"
#include "avpacketqueue.h"

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

class DemuxThread : public MultipleMediaThread
{
public:
    DemuxThread(AvPacketQueue* videoPacketQueue, AvPacketQueue* audioPacketQueue);
    ~DemuxThread();

    bool init(const std::wstring mediaUrl);

    int start();
    int stop();

    AVCodecParameters* getVedioCodecParameters();
    AVCodecParameters* getAudioCodecParameters();

    AVRational getVedioStreamTimebase();
    AVRational getAudioStreamTimebase();

    virtual void run() override;

private:
    std::string m_mediaUrl;
    AVFormatContext* m_pFormatCtx = nullptr;
    int m_nVideoStreamIndex = -1;
    int m_nAudioStreamIndex = -1;

    AvPacketQueue* m_pVideoPacketQueue = nullptr;
    AvPacketQueue* m_pAudioPacketQueue = nullptr;
};

#endif // !__DEMUXTHREAD__
