#include "openglwidget.h"
#include <QOpenGLTexture>
#include <QDateTime>
#include <cmath>
#include <QDebug>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

constexpr float PI = 3.1415926;

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
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    m_pTextureWall->destroy();
    m_pTextureBoard->destroy();

    doneCurrent();
}

void OpenglWidget::initializeGL()
{
    initializeOpenGLFunctions();

    // 立方体顶点数据
    float vertices[] =
    {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    // 地板顶点数据
    float planeVertices[] =
    {
         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,

         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
         5.0f, -0.5f, -5.0f,  2.0f, 2.0f
    };

    float points[] =
    {
        -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, // 左上位置 颜色
        0.5f,  0.5f, 0.0f, 1.0f, 0.0f, // 右上
        0.5f, -0.5f, 0.0f, 0.0f, 1.0f, // 右下
        -0.5f, -0.5f, 1.0f, 1.0f, 0.0f  // 左下
    };

    float instanceVertices[] = {
        // 位置          // 颜色
        -0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
         0.05f, -0.05f,  0.0f, 1.0f, 0.0f,
        -0.05f, -0.05f,  0.0f, 0.0f, 1.0f,

        -0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
         0.05f, -0.05f,  0.0f, 1.0f, 0.0f,
         0.05f,  0.05f,  0.0f, 1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_wVAO);
    glBindVertexArray(m_wVAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &m_bVAO);
    glBindVertexArray(m_bVAO);

    unsigned int bVBO;
    glGenBuffers(1, &bVBO);
    glBindBuffer(GL_ARRAY_BUFFER, bVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // 几何着色器
    glGenVertexArrays(1, &m_pVAO);
    glBindVertexArray(m_pVAO);

    unsigned int pVBO;
    glGenBuffers(1, &pVBO);
    glBindBuffer(GL_ARRAY_BUFFER, pVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // 实例化着色器
    glGenVertexArrays(1, &m_instanceVAO);
    glBindVertexArray(m_instanceVAO);

    unsigned int instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(instanceVertices), instanceVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

        //方法2，将偏移量设置为顶点属性
    QVector2D translations[100];
    int index = 0;
    float offset = 0.1f;
    for(int y = -10; y < 10; y += 2)
    {
        for(int x = -10; x < 10; x += 2)
        {
            QVector2D translation;
            translation.setX((float)x / 10.0f + offset);
            translation.setY((float)y / 10.0f + offset);
            translations[index++] = translation;
        }
    }

    unsigned int offsetsVBO;
    glGenBuffers(1, &offsetsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, offsetsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(translations), translations, GL_STATIC_DRAW);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(2, 1);

    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shader.vert");
    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shader.frag");
    m_pShaderProgram.link();

    m_pShaderProgramStencil.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shader.vert");
    m_pShaderProgramStencil.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shaderstencil.frag");
    m_pShaderProgramStencil.link();

    m_pGeomShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/geomshader.vert");
    m_pGeomShaderProgram.addShaderFromSourceFile(QOpenGLShader::Geometry, ":/shaders/geomshader.geom");
    m_pGeomShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/geomshader.frag");
    m_pGeomShaderProgram.link();

    m_pInstanceShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/instanceshader.vert");
    m_pInstanceShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/instanceshader.frag");
    m_pInstanceShaderProgram.link();

    // 离屏MSAA实践
    //绑定帧缓冲对象
    glGenFramebuffers(1, &m_fBufO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fBufO);

    // 多重采样纹理附件
    m_pTextureFrameBuf = new QOpenGLTexture(QOpenGLTexture::Target2DMultisample);
    m_pTextureFrameBuf->create();
    m_pTextureFrameBuf->bind(0);
    m_pTextureFrameBuf->setFormat(QOpenGLTexture::RGBFormat);
    m_pTextureFrameBuf->setSize(width(), height());
    m_pTextureFrameBuf->setSamples(4);
    m_pTextureFrameBuf->allocateStorage();

    // 绑定附件在帧缓冲对象
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_pTextureFrameBuf->textureId(), 0);

    // 多重采样渲染缓冲对象，不用于采样，只用于数据交换，会更快
    glGenRenderbuffers(1, &m_rBuf);
    glBindRenderbuffer(GL_RENDERBUFFER, m_rBuf);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width(), height());

    // 绑定附件在帧缓冲对象
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rBuf);

    // 检查帧缓冲是否完整
    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << framebufferStatus;
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());// 切换回默认缓冲中

    m_pTextureWall = new QOpenGLTexture(QImage(":/images/wall.jpg").mirrored());
    m_pTextureBoard = new QOpenGLTexture(QImage(":/images/board.png").mirrored());

    m_pShaderProgram.bind();
    m_pShaderProgram.setUniformValue("texture", 0);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
}

void OpenglWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void OpenglWidget::paintGL()
{
    /*几何着色器使用代码
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw points
    m_pGeomShaderProgram.bind();
    glBindVertexArray(m_pVAO);
    glDrawArrays(GL_POINTS, 0, 4);
    */

    /* 实例化技术的使用，主要处理大量重复的模型
    //1、使用gl_InstanceID，有局限性，当数量很多的时候会超出uiniform数组的元素个数上限
    QVector2D translations[100];
    int index = 0;
    float offset = 0.1f;
    for(int y = -10; y < 10; y += 2)
    {
        for(int x = -10; x < 10; x += 2)
        {
            QVector2D translation;
            translation.setX((float)x / 10.0f + offset);
            translation.setY((float)y / 10.0f + offset);
            translations[index++] = translation;
        }
    }
    m_pInstanceShaderProgram.bind();
    // 获取uniforms数组offsets位置
    GLint offsetLocation = m_pInstanceShaderProgram.uniformLocation("offsets");
    // 将translations数组的值传递给uniform数组
    m_pInstanceShaderProgram.setUniformValueArray(offsetLocation, translations, 100);
    glBindVertexArray(m_instanceVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 100);

     //2、避免方法1的局限性，使用顶点属性的方法来传递，这样能够传递的数量就完全可以支撑业务了
     m_pInstanceShaderProgram.bind();
     glBindVertexArray(m_instanceVAO);
     glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 100);
     */

    /*模板测试代码
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // 设置mvp变换矩阵
    QMatrix4x4 model = QMatrix4x4();
    QMatrix4x4 view;
    QMatrix4x4 projection;

    m_pShaderProgram.bind();
    view.lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_up);
    projection.perspective(m_fov, (float)width() / height(), 0.1, 100);
    m_pShaderProgram.setUniformValue("view", view);
    m_pShaderProgram.setUniformValue("projection", projection);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 画地板
    glStencilMask(0x00);// 不让画地板影响到模板值
    glBindVertexArray(m_bVAO);
    m_pTextureBoard->bind(0);
    model = QMatrix4x4();
    m_pShaderProgram.setUniformValue("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 画立方体是允许写入，并将绘制过的设为1
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    // 画第一个立方体
    model.translate(-1.0f, 0.0f, -1.0f);
    m_pShaderProgram.setUniformValue("model", model);
    glBindVertexArray(m_wVAO);
    m_pTextureWall->bind(0);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // 画第二个立方体
    model = QMatrix4x4();
    model.translate(2.0f, 0.0f, 0.0f);
    m_pShaderProgram.setUniformValue("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    m_pTextureWall->release();
    m_pTextureBoard->release();

    // 画描边，禁止写入，设置通过模板测试行为,同时禁用深度测试
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00);
    glDisable(GL_DEPTH_TEST);
    m_pShaderProgramStencil.bind();
    glBindVertexArray(m_wVAO);

    float scale = 1.1f;
    m_pShaderProgramStencil.setUniformValue("view", view);
    m_pShaderProgramStencil.setUniformValue("projection", projection);

    model = QMatrix4x4();
    model.translate(-1.0f, 0.0f, -1.0f);
    model.scale(scale, scale, scale);
    m_pShaderProgramStencil.setUniformValue("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    model = QMatrix4x4();
    model.translate(2.0f, 0.0f, 0.0f);
    model.scale(scale, scale, scale);
    m_pShaderProgramStencil.setUniformValue("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilMask(0xFF);
    glEnable(GL_DEPTH_TEST);
    */

    /* MSAA的使用代码*/
    // MSAA抗锯齿使用，需要在创建opgl窗口时就设置MASS
    glBindFramebuffer(GL_FRAMEBUFFER, m_fBufO);
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 设置mvp变换矩阵
    QMatrix4x4 model = QMatrix4x4();
    model.rotate(45, 0.0f, 1.0f, 0.0f);
    QMatrix4x4 view;
    QMatrix4x4 projection;

    m_pShaderProgram.bind();
    view.lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_up);
    projection.perspective(m_fov, (float)width() / height(), 0.1, 100);
    m_pShaderProgram.setUniformValue("model", model);
    m_pShaderProgram.setUniformValue("view", view);
    m_pShaderProgram.setUniformValue("projection", projection);

    glBindVertexArray(m_wVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    glDisable(GL_DEPTH_TEST);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
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
    // 滚轮控制模型的大小
    if (m_fov >= 1 && m_fov <= 75)
        m_fov -= event->angleDelta().y() / 120;

    if (m_fov < 1) m_fov = 1;
    if (m_fov > 75) m_fov = 75;
}
