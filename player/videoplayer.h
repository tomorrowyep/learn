#ifndef VideoPlayer_H
#define VideoPlayer_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <memory>
#include <QImage>

#include "avframequeue.h"
#include "avsync.h"

class QOpenGLTexture;
class QTimer;
class FfmpegParse;

class VideoPlayer : public QOpenGLWidget, public QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit VideoPlayer(QWidget *parent = nullptr);
    ~VideoPlayer();

    void initVedio(AvSync* sync, AVRational timeBase, AvFrameQueue* pFrameQue, int width, int height);
    void start();
    void stop();

protected:
    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;

    virtual void resizeEvent(QResizeEvent *event) override;

signals:
    void updateSLide(double pts);

private:
    // 视频处理
    void renderVideo();
    QImage yuv2RGB(const AVFrame* frame);
    bool allocateRgbBuf();

private:
    unsigned int m_nVAO = 0;
    unsigned int m_nVBO = 0;
    QOpenGLShaderProgram m_pShaderProgram;
    QOpenGLTexture *m_pTextureVideo = nullptr;
    QTimer *m_time = nullptr;
    QImage m_image;// 存储上一帧的画面，当视频快的时候渲染上一帧，不然会导致画面闪烁
    bool m_isRenderLastImg = false;
    bool m_stop = false;

    AvSync* m_avSync = nullptr;
    AVRational m_timeBase;

    int m_width = 0;
    int m_height = 0;
    int m_dstWidth = 0;
    int m_dstHeight = 0;

    SwsContext* m_swsCtx = nullptr;
    AvFrameQueue* m_pFrameQue = nullptr;
    AVFrame *m_rgbFrame = nullptr;
    uint8_t* m_pRgbBuffer = nullptr;
};

#endif // VideoPlayer_H
