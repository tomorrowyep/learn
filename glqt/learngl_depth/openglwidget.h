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
    unsigned int m_wVAO = 0;
    unsigned int m_bVAO = 0;
    unsigned int m_pVAO = 0;
    QOpenGLShaderProgram m_pShaderProgram;
    QOpenGLShaderProgram m_pShaderProgramStencil;
    QOpenGLShaderProgram m_pGeomShaderProgram;
    QOpenGLTexture *m_pTextureWall = nullptr;
    QOpenGLTexture *m_pTextureBoard = nullptr;
    QTimer *m_time = nullptr;

    // 摄像机
    QVector3D m_cameraPos{0.0, 0.0, 3.0};// 摄像机的位置
    QVector3D m_cameraTarget{0.0, 0.0, 0.0};// 摄像机的方向
    QVector3D m_up{0.0, 1.0, 0.0};// 向上的方向

    QVector3D m_cameraFront{0.0, 0.0, -1};// 摄像机前后移动步数
    QVector3D m_cameraRight{0.0, 0.0, -1};// 摄像机左右移动步数
    float m_fov = 45.0;
};

#endif // OPENGLWIDGET_H
