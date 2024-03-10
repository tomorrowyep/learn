#include "openglwidget.h"
#include <QOpenGLTexture>
#include <QDateTime>
#include <cmath>
#include <QDebug>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

const QVector3D lightPos(0.0f, 0.0f, 0.0f);
int cubeMapWidth = 1024;
int cubeMapHeight = 1024;

OpenglWidget::OpenglWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    m_cameraRight = QVector3D::crossProduct(m_up, m_cameraFront);
    m_time = new QTimer(this);
    m_time->start(100);
    connect(m_time, &QTimer::timeout, [this] ()
    {
        update();
    });
}

OpenglWidget::~OpenglWidget()
{
    makeCurrent();
    glBindVertexArray(0);

    doneCurrent();
}

void OpenglWidget::initializeGL()
{
    initializeOpenGLFunctions();

    // 地板纹理
    m_woodTexture = new QOpenGLTexture(QImage(":/images/board.png").mirrored());
    // 立方体纹理
    m_cubeTexture = new QOpenGLTexture(QImage(":/images/container.png").mirrored());

    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shader.vert");
    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shader.frag");
    m_pShaderProgram.link();
    m_pShaderProgram.bind();
    m_pShaderProgram.setUniformValue("depthMap", 0);
    m_pShaderProgram.setUniformValue("diffuseTexture", 1);
    m_pShaderProgram.release();

    m_pFrameBufShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/framebufshader.vert");
    m_pFrameBufShaderProgram.addShaderFromSourceFile(QOpenGLShader::Geometry, ":/shaders/framebufshader.geom");
    m_pFrameBufShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/framebufshader.frag");
    m_pFrameBufShaderProgram.link();

    // 创建帧缓冲，存储深度信息
    glGenFramebuffers(1, &m_fBufO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fBufO);

    // 设置颜色缓冲附件
    loadCubeMap();
    // 绑定附件在帧缓冲对象
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_fSkyDepthTexture->textureId(), 0);

    // 检查帧缓冲是否完整
    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << framebufferStatus;
    // 切换回默认缓冲中
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void OpenglWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void OpenglWidget::paintGL()
{
    // 阴影实现第一个阶段：将深度信息缓存到帧缓冲中
    glBindFramebuffer(GL_FRAMEBUFFER, m_fBufO);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, cubeMapWidth, cubeMapHeight);
    float far_plane = 25.0f;
    // 设置立方体贴图每个面的vp矩阵
    QMatrix4x4 shadowProjection;
    shadowProjection.perspective(90.0f, (float)cubeMapWidth / cubeMapHeight, 1.0f, 25.0f);

    QMatrix4x4 tmp;
    tmp.lookAt(lightPos, lightPos + QVector3D(1.0,0.0,0.0), QVector3D(0.0,-1.0,0.0));
    m_shadowTransforms.push_back(shadowProjection * tmp);
    tmp = QMatrix4x4();
    tmp.lookAt(lightPos, lightPos + QVector3D(-1.0,0.0,0.0), QVector3D(0.0,-1.0,0.0));
    m_shadowTransforms.push_back(shadowProjection * tmp);
    tmp = QMatrix4x4();
    tmp.lookAt(lightPos, lightPos + QVector3D(0.0,1.0,0.0), QVector3D(0.0,0.0,1.0));
    m_shadowTransforms.push_back(shadowProjection * tmp);
    tmp = QMatrix4x4();
    tmp.lookAt(lightPos, lightPos + QVector3D(0.0,-1.0,0.0), QVector3D(0.0,0.0,-1.0));
    m_shadowTransforms.push_back(shadowProjection * tmp);
    tmp = QMatrix4x4();
    tmp.lookAt(lightPos, lightPos + QVector3D(0.0,0.0,1.0), QVector3D(0.0,-1.0,0.0));
    m_shadowTransforms.push_back(shadowProjection * tmp);
    tmp = QMatrix4x4();
    tmp.lookAt(lightPos, lightPos + QVector3D(0.0,0.0,-1.0), QVector3D(0.0,-1.0,0.0));
    m_shadowTransforms.push_back(shadowProjection * tmp);

    m_pFrameBufShaderProgram.bind();
    GLint offsetLocation = m_pFrameBufShaderProgram.uniformLocation("shadowMatrices");
    m_pFrameBufShaderProgram.setUniformValueArray(offsetLocation, &m_shadowTransforms[0], 6);
    m_pFrameBufShaderProgram.setUniformValue("lightPos", lightPos);
    m_pFrameBufShaderProgram.setUniformValue("far_plane", far_plane);
    renderScene(m_pFrameBufShaderProgram);

    // 切换回默认缓冲中，阴影实现第二个阶段，正常渲染，利用帧缓冲中的深度信息判断是都处于阴影中来绘制影响
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    // reset viewport
    glViewport(0, 0, width(), height());
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_pShaderProgram.bind();
    QMatrix4x4 view;
    QMatrix4x4 projection;
    view.lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_up);
    projection.perspective(m_fov, (float)width() / height(), 0.1, 100);

    // 阴影实现第二个阶段，正常渲染，利用帧缓冲中的深度信息判断是都处于阴影中来绘制影响
    m_pShaderProgram.setUniformValue("far_plane", far_plane);
    m_pShaderProgram.setUniformValue("view", view);
    m_pShaderProgram.setUniformValue("projection", projection);
    m_pShaderProgram.setUniformValue("lightPos", lightPos);
    m_pShaderProgram.setUniformValue("viewPos", m_cameraPos);
    m_fSkyDepthTexture->bind(0);
    m_cubeTexture->bind(1);
    renderScene(m_pShaderProgram);
}

void OpenglWidget::loadCubeMap()
{
    m_fSkyDepthTexture = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
    m_fSkyDepthTexture->create();
    m_fSkyDepthTexture->bind(0);
    m_fSkyDepthTexture->setSize(cubeMapWidth, cubeMapHeight);
    m_fSkyDepthTexture->setFormat(QOpenGLTexture::D32);
    m_fSkyDepthTexture->allocateStorage();

    std::vector<float> depthData(cubeMapWidth * cubeMapHeight, 0.0f);
    for (int i = 0; i < 6; i++)
    {
        QOpenGLTexture::CubeMapFace currentFace = static_cast<QOpenGLTexture::CubeMapFace>(QOpenGLTexture::CubeMapPositiveX + i);
        m_fSkyDepthTexture->setData(0, 0, currentFace,
                                    QOpenGLTexture::Depth, QOpenGLTexture::Float32,
                                    (const void*)depthData.data(), nullptr);
    }

    // 设置纹理参数
    m_fSkyDepthTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_fSkyDepthTexture->setMinificationFilter(QOpenGLTexture::Linear);
    m_fSkyDepthTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
}

void OpenglWidget::renderScene(QOpenGLShaderProgram &shader)
{
    // Room cube
    QMatrix4x4 model;
    model.scale(QVector3D(10.0, 10.0, 10.0));
    shader.setUniformValue("model", model);
    glDisable(GL_CULL_FACE); // Note that we disable culling here since we render 'inside' the cube instead of the usual 'outside' which throws off the normal culling methods.
    renderCube();
    glEnable(GL_CULL_FACE);
    // Cubes
    model = QMatrix4x4();
    model.translate(QVector3D(4.0f, -3.5f, 0.0));
    shader.setUniformValue("model", model);
    renderCube();
    model = QMatrix4x4();
    model.translate(QVector3D(2.0f, 3.0f, 1.0));
    model.scale(QVector3D(1.5, 1.5, 1.5));
    shader.setUniformValue("model", model);
    renderCube();
    model = QMatrix4x4();
    model.translate(QVector3D(-3.0f, -1.0f, 0.0));
    shader.setUniformValue("model", model);
    renderCube();
    model = QMatrix4x4();
    model.translate(QVector3D(-1.5f, 1.0f, 1.5));
    shader.setUniformValue("model", model);
    renderCube();
    model = QMatrix4x4();
    model.translate(QVector3D(-1.5f, 2.0f, -3.0));
    model.rotate(60.0f, QVector3D(1.0, 0.0, 1.0).normalized());
    model.scale(QVector3D(1.5, 1.5, 1.5));
    shader.setUniformValue("model", model);
    renderCube();
}

GLuint cubeVAO = 0;
GLuint cubeVBO = 0;
void OpenglWidget::renderCube()
{
    // Initialize (if necessary)
    if (cubeVAO == 0)
    {
        GLfloat vertices[] = {
            // Back face
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // Bottom-left
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // top-right
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,  // top-right
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,  // bottom-left
            -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,// top-left
            // Front face
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,  // bottom-right
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,  // top-right
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // top-right
            -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // top-left
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // bottom-left
            // Left face
            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-right
            -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-left
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom-left
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
            -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // bottom-right
            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-right
            // Right face
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-left
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-right
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom-right
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // top-left
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // bottom-left
            // Bottom face
            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
            0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // top-left
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,// bottom-left
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom-left
            -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom-right
            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
            // Top face
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,// top-left
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // top-right
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,// top-left
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // Fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // Link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // Render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void OpenglWidget::keyPressEvent(QKeyEvent *event)
{
    double speed = 2.5 * 100 / 1000;
    switch(event->key())
    {
    case Qt::Key_W:
        m_cameraPos += speed * m_cameraFront;
        break;
    case Qt::Key_S:
        m_cameraPos -= speed * m_cameraFront;
        break;
    case Qt::Key_A:
        m_cameraPos -= speed * m_cameraRight;
        break;
    case Qt::Key_D:
        m_cameraPos += speed * m_cameraRight;
        break;
    default:
        break;
    }
}

void OpenglWidget::mouseMoveEvent(QMouseEvent *event)
{
    // 通过鼠标控制模型的移动
    /*static float pitch = 0;
    static float yaw = -90;
    static QPoint lastPos(width() / 2, height() / 2);
    float sensitivity = 0.1f;

    auto curPos = event->pos();
    auto deltaPos = (curPos - lastPos) * sensitivity;
    lastPos = curPos;

    pitch -= deltaPos.y();// 因为Qt坐标系与GL坐标系y轴是相反的
    yaw += deltaPos.x();

    if (pitch > 89.0) pitch = 89.0;
    if (pitch < -89.0) pitch = -89.0;

    m_cameraFront.setX(cos(yaw * PI / 180) * cos(pitch * PI / 180));
    m_cameraFront.setY(sin(pitch * PI / 180));
    m_cameraFront.setZ(sin(yaw * PI / 180) * cos(pitch * PI / 180));
    m_cameraFront.normalize();
    update();*/
    Q_UNUSED(event);
}

void OpenglWidget::wheelEvent(QWheelEvent *event)
{
    // 滚轮控制模型的缩小和放大
    if (m_fov >= 1 && m_fov <= 75)
        m_fov -= event->angleDelta().y() / 120;

    if (m_fov < 1) m_fov = 1;
    if (m_fov > 75) m_fov = 75;
}

