#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

#include "avsync.h"

class VideoPlayer;
class AvPacketQueue;
class AvFrameQueue;
class DemuxThread;
class DecodeThread;
class AudioPlayerManager;

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void initPlayer();

private:
    // 缓存队列
    AvPacketQueue* m_audioPacketQue = nullptr;
    AvPacketQueue* m_vedioPacketQue = nullptr;
    AvFrameQueue* m_audioFrameQue = nullptr;
    AvFrameQueue* m_vedioFrameQue = nullptr;

    // 解复用
    DemuxThread* m_demuxThread = nullptr;

    // 解码
    DecodeThread* m_audioDecodeThread = nullptr;
    DecodeThread* m_vedioDecodeThread = nullptr;

    // 音视频渲染
    AvSync m_syncClock;
    VideoPlayer* m_pOpglWidget = nullptr;
    AudioPlayerManager* m_audioPlayer = nullptr;
};
#endif // WIDGET_H
