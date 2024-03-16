#include "openglwidget.h"
#include <QOpenGLTexture>
#include <QDateTime>
#include <cmath>
#include <QDebug>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

std::vector<QVector3D> lightPositions
{
    QVector3D(0.0f, 0.5f, 1.5f),// back light
    QVector3D(-4.0f, 0.5f, -3.0f),
    QVector3D(3.0f, 0.5f, 1.0f),
    QVector3D(-0.8f, 2.4f, -1.0f)
};

std::vector<QVector3D> lightColors
{
    QVector3D(5.0f, 5.0f, 5.0f),
    QVector3D(10.0f, 0.0f, 0.0f),
    QVector3D(0.0f, 0.0f, 15.0f),
    QVector3D(0.0f, 5.0f, 0.0f)
};

GLfloat exposure = 1.0f;
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

    // 立方体纹理
    m_woodTexture = new QOpenGLTexture(QImage(":/images/container.png").mirrored());
    m_boardTexture = new QOpenGLTexture(QImage(":/images/board.png").mirrored());

    // 创建帧缓冲，存储hdr图片
    glGenFramebuffers(1, &m_fBufO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fBufO);

    // 设置颜色缓冲附件并绑定附件在帧缓冲对象
    // 设置location，渲染到对应的颜色缓冲附件
    GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    createAttatch(m_fHdrTexture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fHdrTexture->textureId(), 0);

    createAttatch(m_hightLightTexture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 1, GL_TEXTURE_2D, m_hightLightTexture->textureId(), 0);

     // 绑定渲染缓冲对象
    GLuint rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width(), height());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    // 检查帧缓冲是否完整
    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << framebufferStatus;

    // 创建高斯模糊的帧缓冲与颜色附件（水平和垂直方向）
    glGenFramebuffers(2, m_fBoolmBufs);
    for (int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fBoolmBufs[i]);

        if (i == 0)
        {
            createAttatch(m_bloomHTexture);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_bloomHTexture->textureId(), 0);
        }
        else
        {
            createAttatch(m_bloomVTexture);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_bloomVTexture->textureId(), 0);
        }

        GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
            qDebug() << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << framebufferStatus;
    }

    // 切换回默认缓冲中
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shader.vert");
    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shader.frag");
    m_pShaderProgram.link();
    m_pShaderProgram.bind();
    m_pShaderProgram.setUniformValue("scene", 0);
    m_pShaderProgram.setUniformValue("bloomBlur", 1);// 这里有个小坑，就是set之后不是马上发给GPU，在draw时才会发送，因此在没有绘制前都不要release掉，不然没有设置进去

    m_pBloomShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/bloomshader.vert");
    m_pBloomShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/bloomshader.frag");
    m_pBloomShaderProgram.link();

    m_pFrameBufShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/framebufshader.vert");
    m_pFrameBufShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/framebufshader.frag");
    m_pFrameBufShaderProgram.link();

    glEnable(GL_DEPTH_TEST);
}

void OpenglWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void OpenglWidget::paintGL()
{
    // 泛光第一阶段，渲染获取HDR和提取亮光
    glBindFramebuffer(GL_FRAMEBUFFER, m_fBufO);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 view;
    QMatrix4x4 projection;
    view.lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_up);
    projection.perspective(m_fov, (float)width() / height(), 0.1, 100);
    m_pFrameBufShaderProgram.bind();
    m_pFrameBufShaderProgram.setUniformValue("isLightSource", false);
    m_pFrameBufShaderProgram.setUniformValue("view", view);
    m_pFrameBufShaderProgram.setUniformValue("projection", projection);
    m_boardTexture->bind();

    GLint offsetLocation = m_pFrameBufShaderProgram.uniformLocation("lightPositions");
    m_pFrameBufShaderProgram.setUniformValueArray(offsetLocation, &lightPositions[0], 4);
    offsetLocation = m_pFrameBufShaderProgram.uniformLocation("lightColors");
    m_pFrameBufShaderProgram.setUniformValueArray(offsetLocation, &lightColors[0], 4);

    // 绘制地板
    auto model = QMatrix4x4();
    model.translate(QVector3D(0.0f, -1.0f, 0.0));
    model.scale( QVector3D(12.5f, 0.5f, 12.5f));
    m_pFrameBufShaderProgram.setUniformValue("model", model);
    renderCube();
    m_boardTexture->release();

    // 绘制多个立方体
    m_woodTexture->bind();
    model = QMatrix4x4();
    model.translate(QVector3D(0.0f, 1.5f, 0.0));
    model.scale( QVector3D(0.5f, 0.5f, 0.5f));
    m_pFrameBufShaderProgram.setUniformValue("model", model);
    renderCube();

    model = QMatrix4x4();
    model.translate(QVector3D(2.0f, 0.0f, 1.0));
    model.scale( QVector3D(0.5f, 0.5f, 0.5f));
    m_pFrameBufShaderProgram.setUniformValue("model", model);
    renderCube();

    model = QMatrix4x4();
    model.translate(QVector3D(-1.0f, -1.0f, 2.0));
    model.rotate(60.0f, QVector3D(1.0, 0.0, 1.0));
    m_pFrameBufShaderProgram.setUniformValue("model", model);
    renderCube();

    model = QMatrix4x4();
    model.translate(QVector3D(0.0f, 2.7f, 4.0));
    model.rotate(23.0f, QVector3D(1.0, 0.0, 1.0));
    model.scale( QVector3D(1.25f, 1.25f, 1.25f));
    m_pFrameBufShaderProgram.setUniformValue("model", model);
    renderCube();

    model = QMatrix4x4();
    model.translate(QVector3D(-2.0f, 1.0f, -3.0));
    model.rotate(124.0f, QVector3D(1.0, 0.0, 1.0).normalized());
    m_pFrameBufShaderProgram.setUniformValue("model", model);
    renderCube();

    model = QMatrix4x4();
    model.translate(QVector3D(-3.0f, 0.0f, 0.0));
    model.scale(QVector3D(0.5f, 0.5f, 0.5f));
    m_pFrameBufShaderProgram.setUniformValue("model", model);
    renderCube();
    m_woodTexture->release();

    // 绘制光源立方体
    m_pFrameBufShaderProgram.setUniformValue("isLightSource", true);
    for (int i = 0; i < 4; i++)
    {
        model = QMatrix4x4();
        model.translate(lightPositions[i]);
        model.scale(QVector3D(0.25f, 0.25f, 0.25f));
        m_pFrameBufShaderProgram.setUniformValue("model", model);
        m_pFrameBufShaderProgram.setUniformValue("lightIndex", i);
        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    // 第二阶段，高斯模糊，分别对水平和垂直方向高斯模糊
    GLboolean first_iteration = true;
    GLuint amount = 10, horizontal = 1;
    m_pBloomShaderProgram.bind();
    for (GLuint i = 0; i < amount; i++)
    {
        auto ret = horizontal == 1 ? 0 : 1;
        m_pBloomShaderProgram.setUniformValue("horizontal", horizontal);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fBoolmBufs[ret]);
        if (first_iteration)
        {
            first_iteration = false;
            m_hightLightTexture->bind();
        }
        else
        {
            horizontal == 0 ? m_bloomHTexture->bind() : m_bloomVTexture->bind();
        }

        renderQuad();
        horizontal = ret;
    }

    // 切换回默认缓冲中，第三阶段，混合纹理得到最后的渲染结果
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_pShaderProgram.bind();
    m_pShaderProgram.setUniformValue("bloom", true);
    m_pShaderProgram.setUniformValue("exposure", exposure);
    m_fHdrTexture->bind(1);
    //m_hightLightTexture->bind(0);
    horizontal == 0 ? m_bloomHTexture->bind(0) : m_bloomVTexture->bind(0);
    renderQuad();
}

void OpenglWidget::createAttatch(QOpenGLTexture *&texture)
{
    texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    texture->create();
    texture->bind();
    texture->setFormat(QOpenGLTexture::RGBA16F);
    texture->setSize(width(), height());
    texture->setMinificationFilter(QOpenGLTexture::Linear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    texture->allocateStorage();
}

GLuint quadVAO = 0;
GLuint quadVBO;
void OpenglWidget::renderQuad()
{
    if (quadVAO == 0)
    {
        GLfloat quadVertices[] = {
            // Positions        // Texture Coords
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // Setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void OpenglWidget::renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
            1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
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

