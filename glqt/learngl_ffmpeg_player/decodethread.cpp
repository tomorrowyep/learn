#include "decodethread.h"

#include <iostream>

namespace
{
    constexpr const char* g_hwTypeName = "cuda";
}

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

    // 支持硬件解码就采用硬件解码
    if (isSupportHardwareDecode(paras))
        return true;

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
    AVFrame* hw_frame = av_frame_alloc();// 用于硬件解码的转换
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
                if (frame->format == m_hw_pix_fmt)
                {
                    // 硬解码，解码后的图像在GPU上
                    // 把图像从GPU拷贝到CPU
                    // av_hwframe_transfer_data转换后的格式是NV12的格式转换
                    // 也可以利用CUDA和OpenGL的互操作直接在GPU上渲染，不用再复制到cpu上
                    if ((ret = av_hwframe_transfer_data(hw_frame, frame, 0)) < 0)
                        break;

                    // 只是复制像素信息，pts信息需要手动复制
                    hw_frame->pts = frame->pts;
                    m_frameQue->push(hw_frame);

                    // 释放GPU上的数据
                    av_frame_unref(frame);
                }
                else
                {
                    // 解码之后的图像在CPU上
                    m_frameQue->push(frame);
                }
            }
        }
    }
}

bool DecodeThread::isSupportHardwareDecode(AVCodecParameters* paras)
{
    // 遍历下设备上支持的类型
    AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
    {
        const char *type_name = av_hwdevice_get_type_name(type);
        if (type_name)
           std::cout << type_name << '\n';
    }

    // 获取cuda的硬件解码类型
    type = av_hwdevice_find_type_by_name(g_hwTypeName);
    if (type == AV_HWDEVICE_TYPE_NONE)
        return false;

    const AVCodec* pCodec = nullptr;
    pCodec = avcodec_find_decoder(paras->codec_id);// H264 : avcodec_find_decoder_by_name("h264_cuvid");// H265 : avcodec_find_decoder_by_name("hevc_cuvid");
    if (!pCodec)
        return false;

    // 获取该硬解码器的像素格式。cuda对应的hw_pix_fmt是AV_PIX_FMT_CUDA
    for (int i = 0;; i++)
    {
        const AVCodecHWConfig *config = avcodec_get_hw_config(pCodec, i);
        if (!config)
            return false;

        if ((config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
                config->device_type == type)
        {
            m_hw_pix_fmt = config->pix_fmt;
            break;
        }
    }

    // 获取下格式对应的字符串
    //const char *pixname = av_get_pix_fmt_name(hw_pix_fmt);

    // 初始化解码器
    m_codeCtx = avcodec_alloc_context3(pCodec);
    if (!m_codeCtx)
        return false;

    // 拷贝解码参数
    avcodec_parameters_to_context(m_codeCtx, paras);
    // 设置硬解码格式
    m_codeCtx->pix_fmt = m_hw_pix_fmt;

    // 创建一个硬件设备上下文
    int err = 0;
    AVBufferRef *hw_device_ctx = nullptr;
    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type, NULL, NULL, 0)) < 0)
        return false;

    // 引用技术机制，保证使用期间是没有被释放的
    m_codeCtx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    // 打开解码器
    int ret = avcodec_open2(m_codeCtx, pCodec, NULL);
    if (ret < 0)
       return false;

    // 减少一次引用计数，避免内存泄露
    av_buffer_unref(&hw_device_ctx);
    return true;
}
