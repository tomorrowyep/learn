#include "widget.h"
#include "videoplayer.h"
#include "audioplayer.h"
#include "avpacketqueue.h"
#include "demuxthread.h"
#include "decodethread.h"
#include <QVBoxLayout >
#include <QSlider>

namespace
{
constexpr const wchar_t* videoPath = L"C:/Users/yeteng/Desktop/tools/test/ffmpegtest/res/mp4/oceans.mp4";
constexpr const wchar_t* videoPath02 = (L"C:/Users/yeteng/Desktop/tools/test/ffmpegtest/res/mp4/trailer.mp4");
constexpr int g_maxInterval = 2;
}

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
   this->resize(1000, 800);
   m_pOpglWidget = new VideoPlayer(this);

   m_slider = new QSlider(Qt::Horizontal, this);
   m_slider->setRange(0, 100);

   // 设置已滑过和未滑过区域的颜色
   m_slider->setStyleSheet(R"(
                         QSlider::groove:horizontal {
                         height: 8px;  /* 滑块槽的高度 */
                         background: #cccccc;  /* 未滑过的区域颜色 */
                         border-radius: 4px;
                         }
                         QSlider::sub-page:horizontal {
                         background: #4CAF50;  /* 已滑过的区域颜色 */
                         border-radius: 4px;
                         }
                         QSlider::handle:horizontal {
                         width: 16px;  /* 滑块的宽度 */
                         background: #ffc0cb;
                         border: 1px solid #4CAF50;
                         border-radius: 8px;
                         }
                         )");

   QVBoxLayout *vLayout = new QVBoxLayout;
   vLayout->addWidget(m_pOpglWidget);
   vLayout->addWidget(m_slider);
   this->setLayout(vLayout);

   connect(m_pOpglWidget, &VideoPlayer::updateSLide, this, [this] (double pts)
   {
       if (abs(pts - m_lastPts) >= g_maxInterval)
           return;

       m_slider->setValue(pts);
       m_lastPts = pts;
   });
   connect(m_slider, &QSlider::valueChanged, this, &Widget::updateVideoSchedule);
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
    m_slider->setRange(0, m_demuxThread->getTotalDuration());
    m_demuxThread->start();

    // 解码
    m_audioDecodeThread = new DecodeThread(m_audioPacketQue, m_audioFrameQue);
    m_audioDecodeThread->init(m_demuxThread->getAudioCodecParameters());
    m_audioDecodeThread->start();

    m_vedioDecodeThread = new DecodeThread(m_vedioPacketQue, m_vedioFrameQue);
    m_vedioDecodeThread->init(m_demuxThread->getVedioCodecParameters());
    m_vedioDecodeThread->start();

    // 初始化audio
    m_audioPlayer = new AudioPlayerManager(&m_syncClock, m_demuxThread->getAudioStreamTimebase(), m_audioFrameQue);
    m_audioPlayer->init();
    m_audioPlayer->start();

    // 初始化视频
    m_pOpglWidget->initVedio(&m_syncClock, m_demuxThread->getVedioStreamTimebase(), m_vedioFrameQue,
                             m_demuxThread->getVedioCodecParameters()->width, m_demuxThread->getVedioCodecParameters()->height);
}

void Widget::updateVideoSchedule(int pts)
{
    if (abs(pts - m_lastPts) <= g_maxInterval)
        return;

    m_lastPts = pts;
    m_demuxThread->stop();
    m_pOpglWidget->stop();
    m_audioPlayer->stop();

    m_audioPacketQue->release();
    m_vedioPacketQue->release();
    m_audioFrameQue->release();
    m_vedioFrameQue->release();

    m_demuxThread->updateSchedule(pts);
    m_demuxThread->start();
    m_audioPlayer->start();
    m_pOpglWidget->start();
}


