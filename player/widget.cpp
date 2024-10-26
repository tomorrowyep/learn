#include "widget.h"
#include "videoplayer.h"
#include "audioplayer.h"
#include "avpacketqueue.h"
#include "demuxthread.h"
#include "decodethread.h"
#include <QVBoxLayout >

namespace
{
constexpr const wchar_t* videoPath = L"C:/Users/yeteng/Desktop/tools/test/ffmpegtest/res/mp4/oceans.mp4";
constexpr const wchar_t* videoPath02 = (L"C:/Users/yeteng/Desktop/tools/test/ffmpegtest/res/mp4/trailer.mp4");
}

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
   this->resize(1000, 800);
   m_pOpglWidget = new VideoPlayer(this);
   QVBoxLayout *vLayout = new QVBoxLayout;
   vLayout->addWidget(m_pOpglWidget);
   this->setLayout(vLayout);
}

Widget::~Widget()
{
    if (m_audioPlayer)
        m_audioPlayer->stop();
    if (m_demuxThread)
        m_demuxThread->stop();
    if (m_audioDecodeThread)
        m_audioDecodeThread->stop();
    if (m_vedioDecodeThread)
        m_vedioDecodeThread->stop();

    delete m_vedioFrameQue;
    delete m_audioFrameQue;
    delete m_vedioPacketQue;
    delete m_audioPacketQue;
}

void Widget::initPlayer()
{
    m_audioPacketQue = new AvPacketQueue;
    m_vedioPacketQue = new AvPacketQueue;;
    m_audioFrameQue = new AvFrameQueue;
    m_vedioFrameQue = new AvFrameQueue;

    // 解复用
    m_demuxThread = new DemuxThread(m_vedioPacketQue, m_audioPacketQue);
    m_demuxThread->init(videoPath02);
    m_demuxThread->start();

    // 解码
    m_audioDecodeThread = new DecodeThread(m_audioPacketQue, m_audioFrameQue);
    m_audioDecodeThread->init(m_demuxThread->getAudioCodecParameters());
    m_audioDecodeThread->start();

    m_vedioDecodeThread = new DecodeThread(m_vedioPacketQue, m_vedioFrameQue);
    m_vedioDecodeThread->init(m_demuxThread->getVedioCodecParameters());
    m_vedioDecodeThread->start();

    // 初始化视频
    m_pOpglWidget->initVedio(&m_syncClock, m_demuxThread->getVedioStreamTimebase(), m_vedioFrameQue,
                             m_demuxThread->getVedioCodecParameters()->width, m_demuxThread->getVedioCodecParameters()->height);

    // 初始化audio
    m_audioPlayer = new AudioPlayerManager(&m_syncClock, m_demuxThread->getAudioStreamTimebase(), m_audioFrameQue);
    m_audioPlayer->init();
    m_audioPlayer->start();
}


