#include "audioplayer.h"

#include <QIODevice>
#include <QFile>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace
{
// 使用汉明窗设计FIR低通滤波器
void designFIRLowPassFilter(std::vector<double>& filterCoeffs, double cutoffFreq, int filterSize, double sampleRate)
{
    double nyquist = sampleRate / 2.0;  // 奈奎斯特频率
    double normCutoffFreq = cutoffFreq / nyquist;

    int M = filterSize - 1;
    for (int i = 0; i <= M; ++i)
    {
        if (i == M / 2)
        {
            filterCoeffs[i] = normCutoffFreq;
        }
        else
        {
            double n = i - M / 2.0;
            filterCoeffs[i] = normCutoffFreq * (sin(2.0 * M_PI * normCutoffFreq * n) / (M_PI * n));
        }
        // 使用汉明窗进行加权
        filterCoeffs[i] *= 0.54 - 0.46 * cos(2.0 * M_PI * i / M);
    }
}

// 应用FIR滤波器的函数
void applyFIRFilterStereo(uint8_t* audioBuffer, int bufferSize, const std::vector<double>& filterCoeffs)
{
    int16_t* buffer = reinterpret_cast<int16_t*>(audioBuffer);
    int totalSamples = bufferSize / sizeof(int16_t);  // 样本总数
    int channelSamples = totalSamples / 2;  // 每个声道的样本数量

    std::vector<int16_t> leftChannel(channelSamples);
    std::vector<int16_t> rightChannel(channelSamples);

    // 将双声道分离到左右声道缓冲区
    for (int i = 0; i < channelSamples; ++i)
    {
        leftChannel[i] = buffer[2 * i];
        rightChannel[i] = buffer[2 * i + 1];
    }

    // 滤波后的数据
    std::vector<int16_t> filteredLeftChannel(channelSamples);
    std::vector<int16_t> filteredRightChannel(channelSamples);

    int filterSize = filterCoeffs.size();
    int halfSize = filterSize / 2;

    // 应用FIR滤波器进行滤波
    for (int i = halfSize; i < channelSamples - halfSize; ++i)
    {
        double leftSum = 0.0;
        double rightSum = 0.0;

        for (int j = 0; j < filterSize; ++j)
        {
            leftSum += leftChannel[i - halfSize + j] * filterCoeffs[j];
            rightSum += rightChannel[i - halfSize + j] * filterCoeffs[j];
        }

        filteredLeftChannel[i] = std::clamp(static_cast<int16_t>(leftSum), (int16_t)-32768, (int16_t)32767);
        filteredRightChannel[i] = std::clamp(static_cast<int16_t>(rightSum), (int16_t)-32768, (int16_t)32767);
    }

    // 将滤波后的结果拷贝回缓冲区
    for (int i = 0; i < channelSamples; ++i)
    {
        buffer[2 * i] = filteredLeftChannel[i];
        buffer[2 * i + 1] = filteredRightChannel[i];
    }
}

void lowPassFIRFilterStereo(uint8_t* audioBuffer, int bufferSize, double cutoffFreq = 3000.0, int filterSize = 101, double sampleRate = 48000.0)
{
    // 设计低通滤波器的系数
    std::vector<double> filterCoeffs(filterSize);
    designFIRLowPassFilter(filterCoeffs, cutoffFreq, filterSize, sampleRate);

    // 应用滤波器到音频信号
    applyFIRFilterStereo(audioBuffer, bufferSize, filterCoeffs);
}

// 使用汉明窗设计FIR高通滤波器
void designFIRHighPassFilter(std::vector<double>& filterCoeffs, double cutoffFreq, int filterSize, double sampleRate)
{
    // 设计低通滤波器系数
    designFIRLowPassFilter(filterCoeffs, cutoffFreq, filterSize, sampleRate);

    // 转换为高通滤波器
    for (int i = 0; i < filterSize; ++i)
    {
        filterCoeffs[i] = -filterCoeffs[i];
    }
    filterCoeffs[filterSize / 2] += 1.0;  // 修正中心系数
}

void highPassFIRFilterStereo(uint8_t* audioBuffer, int bufferSize, double cutoffFreq = 120.0, int filterSize = 101, double sampleRate = 48000.0)
{
    // 设计高通滤波器的系数
    std::vector<double> filterCoeffs(filterSize);
    designFIRHighPassFilter(filterCoeffs, cutoffFreq, filterSize, sampleRate);

    // 应用滤波器到音频信号
    applyFIRFilterStereo(audioBuffer, bufferSize, filterCoeffs);
}

}

AudioPlayerManager::AudioPlayerManager(AvSync* sync, AVRational timeBase, AvFrameQueue* pFrameQue,
                                       AudioParas outParas,  QObject* parent)
    : QObject(parent),
      m_outputParas(outParas),
      m_pFrameQue(pFrameQue),
      m_avSync(sync),
      m_timeBase(timeBase)
{

}

AudioPlayerManager::~AudioPlayerManager()
{
    delete m_audioBuffer;
    m_audioBuffer = nullptr;
}

void AudioPlayerManager::init()
{
    // 设置音频输出到声卡的格式
    m_fmt.setSampleRate(m_outputParas.freq);
    m_fmt.setSampleSize(m_outputParas.sampleSize); // 设置样本大小,一般为s16
    m_fmt.setChannelCount(m_outputParas.channels);
    m_fmt.setCodec("audio/pcm");
    m_fmt.setByteOrder(QAudioFormat::LittleEndian);
    m_fmt.setSampleType(QAudioFormat::SignedInt);

    // 初始化 QAudioOutput 对象
    m_audioOut = new QAudioOutput(m_fmt, this);
    m_audioDevice = m_audioOut->start(); // 开始音频输出

    m_periodSize = m_audioOut->periodSize();
    m_writeIoData = std::make_unique<uint8_t[]>(m_periodSize);
}

int AudioPlayerManager::start()
{
    m_abort = false;
    m_thread = new std::thread(&AudioPlayerManager::run, this);
    if (!m_thread)
        return -1;

    return 0;
}

int AudioPlayerManager::stop()
{
    MultipleMediaThread::stop();
    return 0;
}

void AudioPlayerManager::run()
{
    while (!m_abort)
    {
        playAudio();
    }
}

void AudioPlayerManager::playAudio()
{
    if (m_curIoIndex != m_periodSize)
    {
        if (m_curIndex != 0)
        {
            // 处理m_audioBuffer上次没有copy完的数据
            size_t copyDataSize = std::min(m_periodSize - m_curIoIndex, m_bufferSize - m_curIndex);
            auto writeIoBuf = m_writeIoData.get();
            memcpy(writeIoBuf, m_audioBuffer + m_curIndex, copyDataSize);

            m_curIndex += copyDataSize;
            if (m_curIndex >= m_bufferSize)
                m_curIndex = 0;

            m_curIoIndex += copyDataSize;
            if (m_curIoIndex == m_periodSize)
                goto PLAY_AUDIO;
        }

        AVFrame* frame = m_pFrameQue->pop(10);
        if (!frame)
        {
            // 没有数据或者暂停了
            return;
        }

        // 判断是否需要重采样
        if (frame->format != m_outputParas.fmt
                || frame->sample_rate != m_outputParas.freq
                || frame->ch_layout.nb_channels != m_outputParas.channels)
        {
            bool ret = reSample(frame);
            if (!ret )
            {
                av_frame_free(&frame);
                return;
            }
        }
        else
        {
            int byteCounts = av_samples_get_buffer_size(nullptr, frame->ch_layout.nb_channels, frame->nb_samples, (AVSampleFormat)frame->format, 1);
            av_fast_malloc(&m_audioBuffer, &m_bufferSize, byteCounts);
            m_bufferSize = byteCounts;
            memcpy_s(m_audioBuffer, m_bufferSize, frame->data[0], m_bufferSize);
        }

        m_pts = frame->pts * av_q2d(m_timeBase);
        // 释放数据
        av_frame_free(&frame);

        // 填充需要写入音频的数据
        if (m_curIoIndex != m_periodSize)
        {
            size_t copyDataSize = std::min(m_periodSize - m_curIoIndex, m_bufferSize - m_curIndex);
            auto writeIoBuf = m_writeIoData.get();
            memcpy(writeIoBuf, m_audioBuffer + m_curIndex, copyDataSize);

            m_curIndex += copyDataSize;
            if (m_curIndex >= m_bufferSize)
                m_curIndex = 0;

            m_curIoIndex += copyDataSize;
            if (m_curIoIndex == m_periodSize)
                goto PLAY_AUDIO;
        }

        return;
    }

PLAY_AUDIO:
    // 播放音频，每次写入一个周期的数据
    if (m_audioOut->bytesFree() >= m_audioOut->periodSize())
    {
        m_audioDevice->write((char*)m_writeIoData.get(), m_periodSize);
        m_curIoIndex = 0;

        m_avSync->setClock(m_pts);
    }
}

bool AudioPlayerManager::reSample(const AVFrame* frame)
{
    if (!frame)
        return false;

    // 创建 SwrContext
    swr_alloc_set_opts2(&m_swrCtx,
                        &m_outputParas.channelLayout, m_outputParas.fmt, m_outputParas.freq,
                        &frame->ch_layout, (AVSampleFormat)frame->format, frame->sample_rate,
                        0, nullptr);

    // 初始化 SwrContext
    if (!m_swrCtx || swr_init(m_swrCtx) < 0)
        return false;

    // 样本数量适合采样率成正比的，这里加上256是避免误差导致缓冲区不够
    int outMaxSamples = frame->nb_samples * m_outputParas.freq / frame->sample_rate + 256;
    int outMaxBytes = av_samples_get_buffer_size(nullptr, m_outputParas.channels, outMaxSamples, m_outputParas.fmt, 1);
    av_fast_malloc(&m_audioBuffer, &m_bufferSize, outMaxBytes); // 快速分配内存，内存够就不会去开辟新的内存
    int realSamples = swr_convert(
                m_swrCtx,
                &m_audioBuffer, // 转换后的数据
                outMaxSamples, // 最大样本数量
                (const uint8_t **)frame->extended_data, // 输入数据extended_data和data一样，只是extended_data支持更多通道的数据
                frame->nb_samples // 输入样本数量
                );

    if (realSamples < 0)
    {
        swr_free(&m_swrCtx);
        return false;
    }

    m_bufferSize = av_samples_get_buffer_size(nullptr, m_outputParas.channels, realSamples, m_outputParas.fmt, 1);

    // 处理噪音
    highPassFIRFilterStereo(m_audioBuffer, m_bufferSize);
    lowPassFIRFilterStereo(m_audioBuffer, m_bufferSize);

    swr_free(&m_swrCtx);
    return true;
}
