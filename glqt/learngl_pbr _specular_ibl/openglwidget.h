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
    void loadCubeMap(QOpenGLTexture* &texture, int size);
    void renderSphere();
    void renderCube();

private:
    unsigned int m_hdrTexture = 0; // diffuse_ibl
    unsigned int m_irradianceBuf = 0; //  辐照度帧缓存
    unsigned int m_fCubeMapBuf = 0;
    QOpenGLShaderProgram m_pbrShaderProgram;
    QOpenGLShaderProgram m_cubeMapShaderProgram;
    QOpenGLShaderProgram m_backgroundShaderProgram;
    QOpenGLShaderProgram m_irradianceShaderProgram;
    QOpenGLTexture *m_irradianceMapTexture = nullptr;
    QOpenGLTexture *m_fCubeMapTexture = nullptr;
    std::vector<QMatrix4x4> m_cubeMapViews;
    QTimer *m_time = nullptr;
    std::vector<QVector3D> m_lightPositions;
    std::vector<QVector3D> m_lightColors;

    // 摄像机
    QVector3D m_cameraPos{0.0, 0.0, 3.0};// 摄像机的位置
    QVector3D m_cameraTarget{0.0, 0.0, 0.0};// 摄像机的方向
    QVector3D m_up{0.0, 1.0, 0.0};// 向上的方向

    QVector3D m_cameraFront{0.0, 0.0, -1};// 摄像机前后移动步数
    QVector3D m_cameraRight{0.0, 0.0, -1};// 摄像机左右移动步数
    float m_fov = 45.0;
};

#endif // OPENGLWIDGET_H
