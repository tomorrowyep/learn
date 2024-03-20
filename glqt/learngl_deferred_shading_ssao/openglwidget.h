#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>

class QOpenGLTexture;
class QTimer;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

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
    void createAttatch(QOpenGLTexture *&texture, bool isSignalComponent = false);
    void renderCube();
    void renderQuad();

private:
    unsigned int m_fGBufO = 0;
    unsigned int m_fSSAOBuf = 0;
    unsigned int m_fSSAOBlurBuf = 0;
    unsigned int m_noiseTexture = 0;
    QOpenGLShaderProgram m_pShaderProgram;
    QOpenGLShaderProgram m_pFrameBufShaderProgram;
    QOpenGLShaderProgram m_pShaderLight;
    QOpenGLShaderProgram m_pSSAOShader;
    QOpenGLShaderProgram m_pSSAOBlurShader;
    QOpenGLTexture *m_posTexture = nullptr;
    QOpenGLTexture *m_norTexture = nullptr;
    QOpenGLTexture *m_diffuseAndSpecularTexture = nullptr;
    QOpenGLTexture *m_woodTexture = nullptr;
    QOpenGLTexture *m_SSAOTexture = nullptr;
    QOpenGLTexture *m_SSAOBlurTexture = nullptr;
    QTimer *m_time = nullptr;
    std::vector<QVector3D> m_lightPositions;
    std::vector<QVector3D> m_lightColors;
    std::vector<QVector3D> m_ssaoKernel;

    // 摄像机
    QVector3D m_cameraPos{0.0, 0.0, 5.0};// 摄像机的位置
    QVector3D m_cameraTarget{0.0, 0.0, 0.0};// 摄像机的方向
    QVector3D m_up{0.0, 1.0, 0.0};// 向上的方向

    QVector3D m_cameraFront{0.0, 0.0, -1};// 摄像机前后移动步数
    QVector3D m_cameraRight{0.0, 0.0, -1};// 摄像机左右移动步数
    float m_fov = 45.0;
};

#endif // OPENGLWIDGET_H
