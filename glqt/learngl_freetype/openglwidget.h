#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <map>
#include <memory>

#include "freetypemanager.h"

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
    void initCharacterInfos();
    void renderText(QOpenGLShaderProgram &shader, const std::string text, GLfloat startX, GLfloat startY, const GLfloat scale, const QVector3D color);

private:
    unsigned int m_nVAO = 0;
    unsigned int m_nVBO = 0;
    QOpenGLShaderProgram m_pShaderProgram;
    QOpenGLTexture *m_pTextureWall = nullptr;
    QOpenGLTexture *m_pTextureFace = nullptr;
    QTimer *m_time = nullptr;
    std::unique_ptr<FreeTypeManager> m_ftManager;
    std::map<FT_ULong, FreeTypeManager::CharacterInfo> m_characterInfos; // 存储字符信息

    // 摄像机
    QVector3D m_cameraPos{0.0, 0.0, 3.0};// 摄像机的位置
    QVector3D m_cameraTarget{0.0, 0.0, 0.0};// 摄像机的方向
    QVector3D m_up{0.0, 1.0, 0.0};// 向上的方向

    QVector3D m_cameraFront{0.0, 0.0, -1};// 摄像机前后移动步数
    QVector3D m_cameraRight{0.0, 0.0, -1};// 摄像机左右移动步数
    float m_fov = 45.0;
};

#endif // OPENGLWIDGET_H
