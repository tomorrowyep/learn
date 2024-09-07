#include <iostream>
#include <locale>
#include <codecvt>
#include <QImage>

#include"ffmpegparse.h"
#include "imagescache.h"

FfmpegParse::FfmpegParse(const std::wstring path)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    m_path = myconv.to_bytes(path);
}

FfmpegParse::~FfmpegParse()
{
    // 清理资源
    av_free(m_pBuffer);
    av_packet_free(&m_packet);
    av_frame_free(&m_pFrameRGB);
    av_frame_free(&m_pFrame);
    av_frame_free(&m_pAudioFrame);
    sws_freeContext(m_swsCtx);
    avcodec_free_context(&m_codecContext);
    avcodec_free_context(&m_audioCodecContext);
    avformat_close_input(&m_pFormatCtx);
}

bool FfmpegParse::openLocalFile()
{
    // 分配 AVFormatContext
    m_pFormatCtx = avformat_alloc_context();
    if (m_pFormatCtx == nullptr)
    {
        std::cout << "avformat_alloc_context failed!\n";
        return false;
    }

    // 已被废弃，之前的作用是初始化编解码器、复用器以及解复用器
    // 新版无需手动去注册初始化，而是调用函数时内部再初始化
    // av_register_all();
    int ret = avformat_open_input(&m_pFormatCtx, m_path.c_str(), nullptr, nullptr);
    if (ret)
    {
        std::cout << "avformat_open_input failed!\n";
        return false;
    }

    // 2、解析头文件以及元数据（描述信息）
    // 解析头文件ftyp (File Type Box): 包含文件类型和兼容性信息。
    // 解析元数据moov(Movie Box) : 是 MP4 文件的元数据部分，包含电影信息和每个轨道（流）的描述
    // moov (Movie Box): 是 MP4 文件的元数据部分，包含电影信息和每个轨道（流）的描述。
        /* 
        mvhd(Movie Header Box) : 包含电影的全局信息，如时长和时间基；
        trak(Track Box) : 每个 trak 表示一个流，如视频或音频流，包含该流的所有信息；
        tkhd(Track Header Box) : 包含该流的基本属性，如流类型、时长等；
        mdia(Media Box) : 包含该流的具体媒体信息；
        stbl(Sample Table Box) : 详细描述了流中的样本（如视频帧或音频样本）的时间、大小、类型、位置等信息。
        */
    // 成功时返回非负数，并填充 AVFormatContext 中的流信息；失败时返回负值错误代码。
    ret = avformat_find_stream_info(m_pFormatCtx, nullptr);
    if (ret < 0)
    {
        std::cout << "avformat_find_stream_info failed!\n";
        return false;
    }

    return true;
}

bool FfmpegParse::openURL()
{
    // 媒体流路径（网址），需要设置第四个参数
    AVDictionary* options = nullptr;
    av_dict_set(&options, "timeout", "10000000", 0);  // 设置网络超时时间为10秒
    av_dict_set(&options, "max_delay", "550", 0);
    av_dict_set(&options, "rtsp_transport", "tcp", 0);  // 指定使用 TCP 作为 RTSP 传输协议

    int ret = avformat_open_input(&m_pFormatCtx, m_path.c_str(), nullptr, &options);// 媒体流时需要传入第四个参数
    if (ret)
    {
        std::cout << "avformat_open_input failed!\n";
        return false;
    }

    ret = avformat_find_stream_info(m_pFormatCtx, nullptr);
    if (ret < 0)
    {
        std::cout << "avformat_find_stream_info failed!\n";
        return false;
    }

    return true;
}

size_t FfmpegParse::getDuration()
{
    // 获取总时间，是以时间基(us)为单位，转换为s
    int64_t time = m_pFormatCtx->duration;
    size_t seconds = static_cast<int>(time / 1000000);

    return seconds;
}

int FfmpegParse::getFrameRate()
{
    return m_frameRate;
}

void FfmpegParse::readFrame()
{
    _parseData();
}

void FfmpegParse::readAudio()
{
    // 读音频帧
    AVPacket *packet = av_packet_alloc();
    if (!m_pAudioFrame)
        m_pAudioFrame = av_frame_alloc();

    while (av_read_frame(m_pFormatCtx, packet) == 0)
    {
        if (packet->stream_index == m_audioStreamIndex)
        {
            int ret = avcodec_send_packet(m_audioCodecContext, packet);
            if (ret < 0)
            {
                av_packet_unref(packet);
                std::cout << "avcodec_send_packet failed!\n";
                return;
            }

            while (avcodec_receive_frame(m_audioCodecContext, m_pAudioFrame) == 0)
            {
                size_t dataSize = av_samples_get_buffer_size(NULL, m_audioCodecContext->ch_layout.nb_channels, m_pAudioFrame->nb_samples, m_audioCodecContext->sample_fmt, 1);

                ImagesCache::AudioInfo info{m_pAudioFrame, m_swrCtx, dataSize};
                ImagesCache::instance().addAudioFrame(info);

                av_frame_unref(m_pAudioFrame);
            }
        }

        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    return;
}

int FfmpegParse::getSampleRate()
{
    if (!m_audioCodecContext)
        return 0;

    return m_audioCodecContext->sample_rate;
}

int FfmpegParse::getchannels()
{
    if (!m_audioCodecContext)
        return 0;

    // codecCtx->channels已经废弃，使用这个获取
    return m_audioCodecContext->ch_layout.nb_channels;
}

AVSampleFormat FfmpegParse::getSampleType()
{
    if (!m_audioCodecContext)
        return {};

    return m_audioCodecContext->sample_fmt;
}

bool FfmpegParse::_parseData()
{
    // 分配空间存储每帧图片数据
    if (!_allocateSpace())
        return false;

    // 读帧, m_pFormatCtx不是线程安全的，读取它的时候需要加锁，如果要实现解码音频和视频就要创建两个实例
    // 也可以同时解码音频和视频
    //std::unique_lock<std::mutex>  guard(m_mutex);
    while (av_read_frame(m_pFormatCtx, m_packet) == 0)
    {
        if (m_packet->stream_index == m_videoStreamIndex)
        {
            // avcode_decode_video2函数已经被废弃，被avcodec_send_packet、avcodec_receive_frame代替
            // avcodec_send_packet这个函数用于将编码数据包（AVPacket）发送到解码器进行解码
            // avcodec_receive_frame用于从解码器中接收解码后的帧（AVFrame）
            int ret = avcodec_send_packet(m_codecContext, m_packet);
            if (ret < 0)
            {
                av_packet_unref(m_packet);
                std::cout << "avcodec_send_packet failed!\n";
                return false;
            }

            while (avcodec_receive_frame(m_codecContext, m_pFrame) == 0)
            {
                // 转换图像
                /*ret = sws_scale(m_swsCtx,
                    m_pFrame->data,
                    m_pFrame->linesize,
                    0,
                    m_codecContext->height,
                    m_pFrameRGB->data,
                    m_pFrameRGB->linesize);

                // QImage是类似共享指针的形式，copy提供深拷贝的能力，其他拷贝都是浅拷贝
                auto image = QImage(m_pFrameRGB->data[0],
                                   m_codecContext->width,
                                   m_codecContext->height,
                                   m_pFrameRGB->linesize[0],
                                   QImage::Format_RGB888).copy();*/
                ImagesCache::VideoInfo info{m_pFrame, m_swsCtx, m_width, m_height};
                ImagesCache::instance().addYUVFrame(info);
                av_frame_unref(m_pFrame);
            }
        }
        else if (m_packet->stream_index == m_audioStreamIndex)
        {
            int ret = avcodec_send_packet(m_audioCodecContext, m_packet);
            if (ret < 0)
            {
                av_packet_unref(m_packet);
                std::cout << "avcodec_send_packet failed!\n";
                return false;
            }

            while (avcodec_receive_frame(m_audioCodecContext, m_pFrame) == 0)
            {
                size_t dataSize = av_samples_get_buffer_size(NULL, m_audioCodecContext->ch_layout.nb_channels, m_pFrame->nb_samples, m_audioCodecContext->sample_fmt, 1);

                ImagesCache::AudioInfo info{m_pFrame, m_swrCtx, dataSize};
                ImagesCache::instance().addAudioFrame(info);

                av_frame_unref(m_pFrame);
            }
        }

        av_packet_unref(m_packet);
    }

    return true;
}

void FfmpegParse::_getVideoInfo(AVStream* stream)
{
    if (!stream)
        return;

    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        // 获取附加信息，AV_PKT_DATA_DISPLAYMATRIX表示获取变换矩阵，包括缩放、旋转、位移等信息
        int32_t* displaymatrix = (int32_t*)av_stream_get_side_data(stream, AV_PKT_DATA_DISPLAYMATRIX, nullptr);
        if (displaymatrix)
        {
            int theta = round(av_display_rotation_get(displaymatrix)); // 从变换矩阵中获取旋转角度信息
            m_rotation = (theta + 360) % 360;
        }

        // 获取video宽高
        m_width = stream->codecpar->width;
        m_height = stream->codecpar->height;

        if (stream->avg_frame_rate.den != 0 && stream->avg_frame_rate.num != 0) 
            m_frameRate = stream->avg_frame_rate.num / stream->avg_frame_rate.den;//每秒多少帧

        m_videoFrameCount = stream->nb_frames; //视频帧数
    }
    /*else if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
    {
        m_audioStreamIndex = i;
        m_audioSamplerate = in_stream->codecpar->sample_rate;
        m_audioChannels = in_stream->codecpar->channels;
    }*/
}

bool FfmpegParse::_allocateSpace()
{
    // 分配空间
    m_pFrame = av_frame_alloc();
    m_pFrameRGB = av_frame_alloc();
    if (!m_pFrame || !m_pFrameRGB)
        return false;

    // avpicture_get_size()已被废弃
    /* av_image_get_buffer_size
   enum AVPixelFormat pix_fmt: 图像的像素格式，比如 AV_PIX_FMT_YUV420P。
   int width: 图像的宽度。
   int height: 图像的高度。
   int align: 缓冲区对齐（通常为 1 或 32）。
   返回值: 返回图像数据所需的字节数。
   */
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_codecContext->width, m_codecContext->height, 32);
    if (bufferSize < 0)
    {
        std::cout << "av_image_get_buffer_size failed!\n";
        return false;
    }

    // 分配合适大小的空间
    m_pBuffer = static_cast<uint8_t*>(av_malloc(bufferSize));
    if (m_pBuffer == nullptr)
    {
        std::cout << "av_malloc failed!\n";
        return false;
    }

    // 将缓冲区数据填充到 data 和 linesize 数组中，初始化m_pFrameRGB
    int ret = av_image_fill_arrays(m_pFrameRGB->data, m_pFrameRGB->linesize, m_pBuffer, AV_PIX_FMT_RGB24, m_codecContext->width, m_codecContext->height, 32);
    if (ret < 0)
    {
        std::cout << "av_image_fill_arrays failed!\n";
        return false;
    }

    // 创建图像转换上下文
    // sws_getCachedContext();
    m_swsCtx = sws_getContext(m_codecContext->width, m_codecContext->height, m_codecContext->pix_fmt, // 原图像的宽高以及格式
        m_codecContext->width, m_codecContext->height, AV_PIX_FMT_RGB24, // 输出（转换后）图像的宽高以及格式
        SWS_FAST_BILINEAR, nullptr, nullptr, nullptr); // 缩放算法或方法、 原图像过滤方式以及输出图像过滤方式、过滤的附加参数
    if (!m_swsCtx)
    {
        std::cout << "sws_getContext failed!\n";
        return false;
    }

    // av_init_packet已废弃
    // av_init_packet() 函数用于初始化 AVPacket 结构，将其 data 和 size 字段设置为默认值。
    // 它通常用于在填充 AVPacket 之前准备好一个干净的包，以确保不会有垃圾值。
    // 在较新的 FFmpeg 版本中，使用 av_packet_alloc() 来分配和初始化 AVPacket，然后使用 av_packet_unref() 来重用和清理 AVPacket。
    // AVPacket用于存储编解码器输入和输出的数据包
    m_packet = av_packet_alloc();
    if (!m_packet)
    {
        std::cout << "sws_getContext failed!\n";
        return false;
    }

    return true;
}

bool FfmpegParse::initVideoInfo()
{
    //av_dump_format(m_pFormatCtx, 0, m_path.c_str(), 0);
    // 3、找到对应流的索引进行解码操作

   // 从视频流或者音频流中找到最合适的一个，根据解码器以及比特率、帧率等决定
   /* 参数解释
   ic: 媒体文件的格式上下文。
   type: 指定流的类型（视频、音频、字幕等）。
   wanted_stream_nb: 想要的流的索引，通常设置为 -1。
   related_stream: 与找到的流相关的流的索引，通常设置为 -1。
   decoder_ret: 返回找到的流的解码器指针。
   flags: 保留标志，通常设置为 0。
   返回值： 成功时返回流的索引，失败时返回负数错误码。
   */
   // 找到视频流中的最合适的那一个并获取到编解码的类型，下面的avcodec_find_decoder专门用于获取
    //const AVCodec* pCodec = nullptr;
    m_videoStreamIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    m_audioStreamIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);// 音频
    if (m_videoStreamIndex < 0 || m_audioStreamIndex < 0)
    {
        std::cout << "avformat_find_stream_info failed!\n";
        return false;
    }

    // 获取视频流和其编码参数，然后找到编解码的类型（这个也可以获取到编解码的类型）
    AVStream* videoStream = m_pFormatCtx->streams[m_videoStreamIndex];
    AVCodecParameters* codecParams = videoStream->codecpar;
    const AVCodec* pCodec = avcodec_find_decoder(codecParams->codec_id);
    // 音频
    AVStream* audioStream = m_pFormatCtx->streams[m_audioStreamIndex];
    AVCodecParameters* audioCodecParams = audioStream->codecpar;
    const AVCodec* pAudioCodec = avcodec_find_decoder(audioCodecParams->codec_id);
    if (!pCodec || !pAudioCodec)
    {
        std::cout << "avcodec_find_decoder failed!\n";
        return false;
    }

    _getVideoInfo(videoStream);

    // 创建编解码器上下文
    m_codecContext = avcodec_alloc_context3(pCodec);
    m_audioCodecContext = avcodec_alloc_context3(pAudioCodec);
    if (!m_codecContext || !m_audioCodecContext)
    {
        std::cout << "avcodec_alloc_context3 failed!\n";
        return false;
    }

    // 将编解码器参数复制到编解码器上下文
    int ret = avcodec_parameters_to_context(m_codecContext, videoStream->codecpar);
    if (ret < 0)
    {
        std::cout << "avcodec_parameters_to_context failed!\n";
        return false;
    }
    // 音频
    ret = avcodec_parameters_to_context(m_audioCodecContext, audioStream->codecpar);
    if (ret < 0)
    {
        std::cout << "avcodec_parameters_to_context failed!\n";
        return false;
    }

    // 设置解码的线程数量
    //m_codecContext->thread_count = 8;

    // 打开编解码器
    ret = avcodec_open2(m_codecContext, pCodec, nullptr);
    if (ret < 0)
    {
        std::cout << "avcodec_open2 failed!\n";
        return false;
    }
    // 音频
    ret = avcodec_open2(m_audioCodecContext, pAudioCodec, nullptr);
    if (ret < 0)
    {
        std::cout << "avcodec_open2 failed!\n";
        return false;
    }

    /* 已经废弃
     * m_swrCtx = swr_alloc_set_opts(
        nullptr,
        av_get_default_channel_layout(m_audioCodecContext->channels),
        AV_SAMPLE_FMT_S16,
        m_audioCodecContext->sample_rate,
        av_get_default_channel_layout(m_audioCodecContext->channels),
        m_audioCodecContext->sample_fmt,
        m_audioCodecContext->sample_rate,
        0, nullptr
    );*/

    // 创建 SwrContext
   m_swrCtx = swr_alloc();
   if (!m_swrCtx)
      return false;

   /*AVChannelLayout channel_layout = {};
   av_channel_layout_default(&channel_layout, m_audioCodecContext->ch_layout.nb_channels);*/

   av_opt_set_int(m_swrCtx, "in_channel_layout", m_audioCodecContext->ch_layout.u.mask, 0);
   av_opt_set_int(m_swrCtx, "in_sample_rate", m_audioCodecContext->sample_rate, 0);
   av_opt_set_sample_fmt(m_swrCtx, "in_sample_fmt", m_audioCodecContext->sample_fmt, 0);

   av_opt_set_int(m_swrCtx, "out_channel_layout", m_audioCodecContext->ch_layout.u.mask, 0);
   av_opt_set_int(m_swrCtx, "out_sample_rate", m_audioCodecContext->sample_rate, 0);
   av_opt_set_sample_fmt(m_swrCtx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

    // 初始化 SwrContext
    if (swr_init(m_swrCtx) < 0)
         return false;

    return true;
}

void FfmpegParse::dshowDevice()
{
    AVFormatContext* pFormatCtx = avformat_alloc_context();

    AVDictionary* options = nullptr;
    av_dict_set(&options, "list_devices", "true", 0); 

    const AVInputFormat* ifm = av_find_input_format("dshow");

    avformat_open_input(&pFormatCtx, "video=dummy", ifm, &options);// 
}

void FfmpegParse::dshowOpt()
{
    AVFormatContext* pFormatCtx = avformat_alloc_context();

    // 设定所需的选项（例如视频设备名称）
    AVDictionary* options = nullptr;
    av_dict_set(&options, "list_options", "true", 0);
    av_dict_set(&options, "video_size", "640x480", 0);  // 设置视频分辨率
    av_dict_set(&options, "framerate", "25", 0);        // 设置帧率

    const AVInputFormat* ifm = av_find_input_format("dshow");

    // 打开设备
    int ret = avformat_open_input(&pFormatCtx, "video=Integrated Webcam", ifm, &options);
    if (ret < 0) 
    {
        std::cerr << "Failed to open input device" << std::endl;
        return;
    }

    // 获取流信息， 后面流程就一样了
   /* ret = avformat_find_stream_info(pFormatCtx, nullptr);
    if (ret < 0) {
        std::cerr << "Failed to retrieve stream info" << std::endl;
        return;
    }*/
}
