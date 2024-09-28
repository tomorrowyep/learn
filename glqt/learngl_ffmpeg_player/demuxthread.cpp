#include <iostream>
#include <locale>
#include <codecvt>

#include"demuxthread.h"

DemuxThread::DemuxThread(AvPacketQueue* videoPacketQueue, AvPacketQueue* audioPacketQueue)
    : m_pVideoPacketQueue(videoPacketQueue), m_pAudioPacketQueue(audioPacketQueue)
{

}

DemuxThread::~DemuxThread()
{
    if (m_pFormatCtx)
        avformat_close_input(&m_pFormatCtx);
}

bool DemuxThread::init(const std::wstring mediaUrl)
{
    // 保存路径
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    m_mediaUrl = myconv.to_bytes(mediaUrl);

    // 分配 AVFormatContext
    m_pFormatCtx = avformat_alloc_context();
    if (!m_pFormatCtx)
        return false;

    int ret = avformat_open_input(&m_pFormatCtx, m_mediaUrl.c_str(), nullptr, nullptr);
    if (ret < 0)
        return false;

    ret = avformat_find_stream_info(m_pFormatCtx, nullptr);
    if (ret < 0)
        return false;

    // 获取音视频流的索引
    m_nVideoStreamIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    m_nAudioStreamIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (m_nVideoStreamIndex < 0 || m_nAudioStreamIndex < 0)
        return false;

    return true;
}

int DemuxThread::start()
{
    m_thread = new std::thread(&DemuxThread::run, this);
    if (!m_thread)
        return -1;

    return 0;
}

int DemuxThread::stop()
{
    MultipleMediaThread::stop();
    return 0;
}

AVCodecParameters *DemuxThread::getVedioCodecParameters()
{
    if (m_nVideoStreamIndex == -1)
        return nullptr;

    return m_pFormatCtx->streams[m_nVideoStreamIndex]->codecpar;
}

AVCodecParameters *DemuxThread::getAudioCodecParameters()
{
    if (m_nAudioStreamIndex == -1)
        return nullptr;

    return m_pFormatCtx->streams[m_nAudioStreamIndex]->codecpar;
}

AVRational DemuxThread::getVedioStreamTimebase()
{
    if (m_nVideoStreamIndex == -1)
        return {};

    return m_pFormatCtx->streams[m_nVideoStreamIndex]->time_base;
}

AVRational DemuxThread::getAudioStreamTimebase()
{
    if (m_nAudioStreamIndex == -1)
        return {};

    return m_pFormatCtx->streams[m_nAudioStreamIndex]->time_base;
}

void DemuxThread::run()
{
    AVPacket packet;
    while (!m_abort)
    {
        // 避免数据量太大
        /*if (m_pVideoPacketQueue->size() > 100 ||  m_pAudioPacketQueue->size() > 100)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }*/

        // packet采用引用计数时，只有当计数为零才释放，av_read_frame会增加计数
        // push接口采用的是数据移动，因此不用在这里释放
        if (av_read_frame(m_pFormatCtx, &packet) == 0)
        {
            if (packet.stream_index == m_nVideoStreamIndex)
                m_pVideoPacketQueue->push(&packet);
            else if (packet.stream_index == m_nAudioStreamIndex)
                m_pAudioPacketQueue->push(&packet);
            else
                av_packet_unref(&packet);
        }
    }
}
