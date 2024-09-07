#include "decodethread.h"

DecodeThread::DecodeThread(AvPacketQueue* pPacketQue, AvFrameQueue* pFrameQue)
    : m_pPacketQue(pPacketQue), m_frameQue(pFrameQue)
{

}

DecodeThread::~DecodeThread()
{
    if (m_codeCtx)
        avcodec_free_context(&m_codeCtx);
}

bool DecodeThread::init(AVCodecParameters* paras)
{
    if (!paras)
        return false;

    const AVCodec* pCodec = avcodec_find_decoder(paras->codec_id);
    if (!pCodec)
        return false;

    m_codeCtx = avcodec_alloc_context3(pCodec);
    if (!m_codeCtx )
        return false;

    int ret = avcodec_parameters_to_context(m_codeCtx, paras);
    if (ret < 0)
        return false;

    // 设置解码的线程数量
    //m_codeCtx->thread_count = 8;

    ret = avcodec_open2(m_codeCtx, pCodec, nullptr);
    if (ret < 0)
        return false;

    return true;
}

int DecodeThread::start()
{
    m_thread = new std::thread(&DecodeThread::run, this);
    if (!m_thread)
        return -1;

    return 0;
}

int DecodeThread::stop()
{
    MultipleMediaThread::stop();
    return 0;
}

void DecodeThread::run()
{
    AVFrame* frame = av_frame_alloc();
    while(!m_abort)
    {
        // 控制数据量，避免占太大内存
        if (m_frameQue->size() > 20)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        AVPacket* packet = m_pPacketQue->pop(10);
        if (packet)
        {
            int ret = avcodec_send_packet(m_codeCtx, packet);
            if (ret < 0)
                break;

            av_packet_free(&packet);

            while (avcodec_receive_frame(m_codeCtx, frame) == 0)
            {
                m_frameQue->push(frame);
            }
        }
    }
}
