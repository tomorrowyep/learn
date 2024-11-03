#include "videoplayer.h"
#include "demuxthread.h"
#include <QOpenGLTexture>
#include <QDateTime>
#include <cmath>
#include <QDebug>
#include <QTimer>
#include <thread>
#include <chrono>
#include <QImage>
#include <QAudioOutput>
#include <QIODevice>
#include <QFile>
#include <QResizeEvent>

constexpr int g_interval = 10;// 10ms触发一次刷新

VideoPlayer::VideoPlayer(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

VideoPlayer::~VideoPlayer()
{
    if (m_rgbFrame)
        av_frame_free(&m_rgbFrame);
    if (m_pRgbBuffer)
        av_free(m_pRgbBuffer);

    makeCurrent();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    doneCurrent();
}

void VideoPlayer::initializeGL()
{
    initializeOpenGLFunctions();

    glGenVertexArrays(1, &m_nVAO);
    glBindVertexArray(m_nVAO);

    glGenBuffers(1, &m_nVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_nVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shader.vert");
    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shader.frag");
    m_pShaderProgram.link();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void VideoPlayer::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void VideoPlayer::paintGL()
{
    if (m_stop)
        return;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_pShaderProgram.bind();
    QMatrix4x4 projection;
    projection.ortho(0.0f, width(), 0.0f, height(), 0.f, 100.f);

    m_pShaderProgram.setUniformValue("projection", projection);
    m_pShaderProgram.setUniformValue("image", 0);

    renderVideo();
}

void VideoPlayer::resizeEvent(QResizeEvent *event)
{
    m_dstWidth = event->size().width();
    m_dstHeight = event->size().height();
    allocateRgbBuf();

    QOpenGLWidget::resizeEvent(event);
}

void VideoPlayer::initVedio(AvSync* sync, AVRational timeBase, AvFrameQueue* pFrameQue, int width, int height)
{
    m_width = width;
    m_height = height;
    m_pFrameQue = pFrameQue;
    m_avSync = sync;
    m_timeBase = timeBase;

    m_dstWidth = this->width();
    m_dstHeight = this->height();
    allocateRgbBuf();

    m_time = new QTimer(this);
    m_time->start(g_interval);
    connect(m_time, &QTimer::timeout, [this] ()
    {
        update();
    });
}

void VideoPlayer::start()
{
    m_stop = false;
}

void VideoPlayer::stop()
{
    m_stop = true;
}

void VideoPlayer::renderVideo()
{
    AVFrame* frame = m_pFrameQue->front();
    if (!frame)
        return;

    double pts = frame->pts * av_q2d(m_timeBase);
    double audioPts =  m_avSync->getClock();
    emit updateSLide(audioPts);// 更新进度条

    double diff = (pts - audioPts) * 1000;// s -> ms
    if (diff > 0)
    {
        if (diff > g_interval)
        {
            m_isRenderLastImg = true;
        }
        else
        {
            // 将 diff 转换为整数毫秒
            auto sleep_duration = std::chrono::milliseconds(static_cast<int64_t>(diff));
            std::this_thread::sleep_for(sleep_duration); // 休眠相差的时间
        }
    }

    if (!m_isRenderLastImg)
    {
        m_image = yuv2RGB(frame);
        frame = m_pFrameQue->pop(10);
        av_frame_free(&frame);
    }

    m_isRenderLastImg = false;

    if (m_image.isNull())
        return;

    // 绑定VAO
    glBindVertexArray(m_nVAO);

    // 创建并绑定纹理
    if (m_pTextureVideo)
    {
        delete m_pTextureVideo; // 释放之前的纹理
        m_pTextureVideo = nullptr;
    }

    m_pTextureVideo = new QOpenGLTexture(m_image.mirrored());
    m_pTextureVideo->setMinificationFilter(QOpenGLTexture::Linear);
    m_pTextureVideo->setMagnificationFilter(QOpenGLTexture::Linear);
    m_pTextureVideo->setWrapMode(QOpenGLTexture::ClampToEdge);
    m_pTextureVideo->bind(0);

    GLfloat leftX = (width() / 2.0) - (m_image.width() / 2.0);
    GLfloat topY = (height() / 2.0) + (m_image.height() / 2.0);
    GLfloat vertices[] =
    {
        leftX, topY,  0.0, 1.0,
        leftX + m_image.width(), topY, 1.0, 1.0,
        leftX, topY - m_image.height(),  0.0, 0.0,

        leftX, topY - m_image.height(), 0.0, 0.0,
        leftX + m_image.width(), topY - m_image.height(), 1.0, 0.0,
        leftX + m_image.width(), topY, 1.0, 1.0
    };

    // 绑定缓冲区并更新数据
    glBindBuffer(GL_ARRAY_BUFFER, m_nVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // 绘制图像
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

QImage VideoPlayer::yuv2RGB(const AVFrame* frame)
{
    if (!frame)
        return QImage();

    m_swsCtx = sws_getContext(m_width, m_height, (AVPixelFormat)frame->format, // 原图像的宽高以及格式
                              m_dstWidth, m_dstHeight, AV_PIX_FMT_RGB24, // 输出（转换后）图像的宽高以及格式
                              SWS_BICUBIC, // 缩放算法
                              nullptr, nullptr, nullptr); // 原图像过滤方式以及输出图像过滤方式、过滤的附加参数
    if (!m_swsCtx)
        return QImage();

    // 将YUV转换为RGB
    sws_scale(m_swsCtx,
              frame->data,
              frame->linesize,
              0,
              m_height,
              m_rgbFrame->data,
              m_rgbFrame->linesize);

     sws_freeContext(m_swsCtx);

    // 将RGB数据转为QImage并显示
    return QImage (m_rgbFrame->data[0],
            m_dstWidth,
            m_dstHeight,
            m_rgbFrame->linesize[0],
            QImage::Format_RGB888).copy();
}

bool VideoPlayer::allocateRgbBuf()
{
    if (m_pRgbBuffer)
    {
        av_free(m_pRgbBuffer);
        m_pRgbBuffer = nullptr;
    }

    if (!m_rgbFrame)
        m_rgbFrame = av_frame_alloc();

    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_dstWidth, m_dstHeight, 32);
    if (bufferSize < 0)
        return false;

    // 分配合适大小的空间
    m_pRgbBuffer = static_cast<uint8_t*>(av_malloc(bufferSize));
    if (m_pRgbBuffer == nullptr)
        return false;

    // 将缓冲区数据填充到 data 和 linesize 数组中，初始化m_pFrameRGB
    int ret = av_image_fill_arrays(m_rgbFrame->data, m_rgbFrame->linesize, m_pRgbBuffer, AV_PIX_FMT_RGB24,  m_dstWidth, m_dstHeight, 32);
    if (ret < 0)
        return false;

    return true;
}

