#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QIODevice>
#include <QFile>
#include <map>
#include <memory>
#include <QAudioOutput>
#include "freetypemanager.h"
#include "imagescache.h"

class QOpenGLTexture;
class QTimer;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class FfmpegParse;

class OpenglWidget : public QOpenGLWidget, QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit OpenglWidget(QWidget *parent = nullptr);
    ~OpenglWidget();

protected:
    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;

    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

private:
    void initCharacterInfos();
    void initFfmpegParse();
    void renderText(QOpenGLShaderProgram &shader, const std::string text, GLfloat startX, GLfloat startY, const GLfloat scale, const QVector3D color);
    void renderVideo();
    void playAudio();
    QImage yuv2RGB(ImagesCache::VideoInfo info);
    bool allocateRgbBuf(const ImagesCache::VideoInfo &info);
    bool cover2Qt(ImagesCache::AudioInfo &info);

private:
    unsigned int m_nVAO = 0;
    unsigned int m_nVBO = 0;
    QOpenGLShaderProgram m_pShaderProgram;
    QOpenGLTexture *m_pTextureWall = nullptr;
    QOpenGLTexture *m_pTextureFace = nullptr;
    QOpenGLTexture *m_pTextureVideo = nullptr;
    QTimer *m_time = nullptr;
    std::unique_ptr<FreeTypeManager> m_ftManager;
    std::map<FT_ULong, FreeTypeManager::CharacterInfo> m_characterInfos; // 存储字符信息

    // Qt音频输出设备
    QAudioOutput *m_audioOut = nullptr;
    QIODevice *m_audioDevice = nullptr; // 开始音频输出
    QTimer *m_audioTime = nullptr;

    // ffmpeg解析
    std::unique_ptr<FfmpegParse> m_ffmpegParse;
    AVFrame *m_rgbFrame = nullptr;
    uint8_t* m_pRgbBuffer = nullptr;

    QAudioFormat m_fmt;
    std::unique_ptr<uint8_t[]> m_audioBuffer;// 音频缓冲区
    int m_bufferSize = 0;
    bool m_isConvert = false;

    // 摄像机
    QVector3D m_cameraPos{0.0, 0.0, 3.0};// 摄像机的位置
    QVector3D m_cameraTarget{0.0, 0.0, 0.0};// 摄像机的方向
    QVector3D m_up{0.0, 1.0, 0.0};// 向上的方向

    QVector3D m_cameraFront{0.0, 0.0, -1};// 摄像机前后移动步数
    QVector3D m_cameraRight{0.0, 0.0, -1};// 摄像机左右移动步数
    float m_fov = 45.0;
};

#endif // OPENGLWIDGET_H
