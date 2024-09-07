#include "imagescache.h"

ImagesCache::~ImagesCache()
{
    // 清理所有缓存的AVFrame
    for (auto frame : m_frameCache)
        av_frame_free(&(frame.frame));

    for (auto frame : m_audioFrameCache)
        av_frame_free(&(frame.frame));
}

ImagesCache& ImagesCache::instance()
{
    static ImagesCache imagesCache;
    return imagesCache;
}

void ImagesCache::addYUVFrame(VideoInfo info)
{
    //std::lock_guard<std::mutex> lock(m_mutex);
    AVFrame* newFrame = av_frame_clone(info.frame);
    info.frame = newFrame;
    m_frameCache.push_back(info);
}

ImagesCache::VideoInfo ImagesCache::getYUVFrame()
{
    if (m_frameCache.empty())
        return VideoInfo{};

    //std::lock_guard<std::mutex> lock(m_mutex);
    auto yuvFrame = m_frameCache.front();
    m_frameCache.pop_front();
    return yuvFrame;
}

void ImagesCache::addAudioFrame(ImagesCache::AudioInfo info)
{
    AVFrame* newFrame = av_frame_clone(info.frame);
    info.frame = newFrame;
    m_audioFrameCache.push_back(info);
}

ImagesCache::AudioInfo ImagesCache::getAudioFrame()
{
    if (m_audioFrameCache.empty())
        return AudioInfo{};

    //std::lock_guard<std::mutex> lock(m_mutex);
    auto audioFrame = m_audioFrameCache.front();
    m_audioFrameCache.pop_front();
    return audioFrame;
}
