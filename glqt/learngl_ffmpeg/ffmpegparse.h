#ifndef __FFMPEGMANAGER__
#define __FFMPEGMANAGER__

#include <string>
#include <mutex>

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

// 采用ffmpeg读取视频文件（MP4等）相关流程示例
// const char* str = avcodec_configuration();返回编译 FFmpeg 时的配置信息
// av_dump_format(m_pFormatCtx, 0, path, 0)输出文件的相关信息，文件的格式、时间，以及各个流的编解码，像素格式等
/* 参数解释：第二个参数和第四个参数
index: 流的索引（通常为 0 表示所有流）。
is_output: 指示是否为输出流（0 表示输入流，1 表示输出流）。
*/

class FfmpegParse
{
public:
    FfmpegParse(const std::wstring path);
    ~FfmpegParse();

    bool initVideoInfo();

    // 打开本地文件，可能包含中文
    bool openLocalFile();

    // 打开流媒体
    bool openURL();

    // 获取时长，s为单位
    size_t getDuration();
    int getFrameRate();

    // 获取帧数据
    void readFrame();

    // 获取音频信息
    void readAudio();

    // 推流视频
    void pushVideoStream();

    int getSampleRate();
    int getchannels();
    AVSampleFormat getSampleType();

private:
    bool _parseData();
    void _getVideoInfo(AVStream* stream);
    bool _allocateSpace();
    
    // 主要用于获取到电脑设备的音视频信息（摄像头等）
    // 打开后和打开一个音视频文件一样操作就可以了
    // ffmpeg -list_devices true -f dshow -i dummy，用于列出所有可用的 DirectShow 设备，并显示它们的详细信息
    // ffmpeg -list_options true -f dshow -i video="Integrated Webcam" 显示名为 "Integrated Webcam" 设备的所有可用选项
    void dshowDevice();
    void dshowOpt();

private:
    AVFormatContext* m_pFormatCtx = nullptr;
    AVCodecContext* m_codecContext = nullptr;
    SwsContext* m_swsCtx = nullptr;
    AVFrame* m_pFrame = nullptr;
    AVFrame * m_pFrameRGB = nullptr;
    AVFrame* m_firstFrame = nullptr;
    uint8_t* m_pBuffer = nullptr;
    AVPacket* m_packet = nullptr;
    std::string m_path;
    bool m_bPasered = false;
    int m_width = 0;
    int m_height = 0;
    int m_frameRate = 0;
    int m_rotation = 0;

    int m_videoStreamIndex = 0;
    int64_t m_videoFrameCount = 0;

    int m_audioStreamIndex = 0;
    AVCodecContext* m_audioCodecContext = nullptr;
    AVFrame* m_pAudioFrame = nullptr;
    SwrContext *m_swrCtx = nullptr;

    std::mutex m_mutex;
};

#endif // !__FFMPEGMANAGER__
