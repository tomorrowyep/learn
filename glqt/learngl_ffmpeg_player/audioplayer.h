#ifndef AUDIOPLAYERMANAGER_H
#define AUDIOPLAYERMANAGER_H

#include "avframequeue.h"
#include "multimediathread.h"
#include "avsync.h"

#include <QAudioOutput>

class QIODevice;

struct AudioParas
{
    int freq = 48000; // 采样频率
    int channels; // 通道数
    AVChannelLayout channelLayout;// 通道布局（立体声等）
    enum AVSampleFormat fmt = AV_SAMPLE_FMT_S16; // PCM数据格式
    int sampleSize = 16; // 采样大小

    AudioParas(int nums = 2) : channels(nums)
    {
        av_channel_layout_default(&channelLayout, nums);
    }
};

class AudioPlayerManager : public QObject, public MultipleMediaThread
{
    Q_OBJECT
public:
    AudioPlayerManager(AvSync* sync, AVRational timeBase, AvFrameQueue* pFrameQue,
                       AudioParas outParas = {}, QObject* parent = nullptr);
    ~AudioPlayerManager();

    void init();
    int start();
    int stop();
    virtual void run() override;

private:
    void playAudio();
    bool reSample(const AVFrame* frame);

private:
    AudioParas m_outputParas;
    AvFrameQueue* m_pFrameQue = nullptr;

    QAudioOutput *m_audioOut = nullptr; // 音频输出管理
    QIODevice *m_audioDevice = nullptr; // 音频设备
    QAudioFormat m_fmt;

    // 同步
    AvSync* m_avSync = nullptr;
    AVRational m_timeBase;
    double m_pts = 0.;

    SwrContext* m_swrCtx = nullptr;
    uint8_t* m_audioBuffer = nullptr;// 音频缓冲区，存储从视频帧获取的大小
    size_t m_bufferSize = 0;
    size_t m_curIndex = 0; // 表示m_audioBuffer未写入音频缓冲区的起始位置

    std::unique_ptr<uint8_t[]> m_writeIoData; // 存储写入音频的周期字节大小
    size_t m_periodSize = 0; // 音频周期大小
    size_t m_curIoIndex = 0;
};

#endif // AUDIOPLAYERMANAGER_H
