#ifndef IMAGESCACHE_H
#define IMAGESCACHE_H

#include <QList>
#include <QImage>
#include <mutex>

#include "ffmpegparse.h"

class ImagesCache
{
public:
    struct VideoInfo
    {
        AVFrame* frame = nullptr;
        SwsContext* swc = nullptr;
        int vedioWight = 0;
        int vedioHeight = 0;
    };

    struct AudioInfo
    {
        AVFrame* frame = nullptr;
        SwrContext* swr = nullptr;
        size_t dataSize = 0;
    };

    ImagesCache() = default;
    ~ImagesCache();

    static ImagesCache& instance();

    void addYUVFrame(VideoInfo info);
    VideoInfo getYUVFrame();

    void addAudioFrame(AudioInfo info);
    AudioInfo getAudioFrame();

private:
    QList<VideoInfo> m_frameCache;
    QList<AudioInfo> m_audioFrameCache;
    std::mutex m_mutex;
};

#endif // IMAGESCACHE_H
