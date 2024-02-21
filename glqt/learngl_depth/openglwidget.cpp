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
QVector<QVector3D> vegetation
{
    // 生成草的位置
   QVector3D(-1.5f,  0.0f, -0.48f),
   QVector3D( 1.5f,  0.0f,  0.51f),
   QVector3D( 0.0f,  0.0f,  0.7f),
   QVector3D(-0.3f,  0.0f, -2.3f),
   QVector3D( 0.5f,  0.0f, -0.6f),
};

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

     for (int i = 0; i < vegetation.size(); i++)
     {
         float distance = (m_cameraPos - vegetation[i]).length();
         sorted[distance] = vegetation[i];
     }

     // 天空盒数据
     m_skyBox.push_back(QImage(":/images/skybox/right.jpg").convertToFormat(QImage::Format_RGB888));
     m_skyBox.push_back(QImage(":/images/skybox/left.jpg").convertToFormat(QImage::Format_RGB888));
     m_skyBox.push_back(QImage(":/images/skybox/top.jpg").convertToFormat(QImage::Format_RGB888));
     m_skyBox.push_back(QImage(":/images/skybox/bottom.jpg").convertToFormat(QImage::Format_RGB888));
     m_skyBox.push_back(QImage(":/images/skybox/front.jpg").convertToFormat(QImage::Format_RGB888));
     m_skyBox.push_back(QImage(":/images/skybox/back.jpg").convertToFormat(QImage::Format_RGB888));
}

OpenglWidget::~OpenglWidget()
{
    makeCurrent();
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &m_wVAO);
    glDeleteVertexArrays(1, &m_bVAO);
    glDeleteVertexArrays(1, &m_gVAO);
    glDeleteFramebuffers(1, &m_fBufO);

    doneCurrent();
}

void OpenglWidget::initializeGL()
{
    initializeOpenGLFunctions();

    // 立方体顶点数据
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
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

    // 草的顶点位置和纹理
    float transparentVertices[] =
    {
          0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
          0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
          1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

          0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
          1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
          1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    float quadVertices[] =
    {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f
    };

    // 天空盒的顶点数据，绘制在立方体上的
    float skyboxVertices[] = {
        // positions
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &m_wVAO);
    glBindVertexArray(m_wVAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
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

    glGenVertexArrays(1, &m_gVAO);
    glBindVertexArray(m_gVAO);

    unsigned int gVBO;
    glGenBuffers(1, &gVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &m_qVAO);
    glBindVertexArray(m_qVAO);

    unsigned int qVBO;
    glGenBuffers(1, &qVBO);
    glBindBuffer(GL_ARRAY_BUFFER, qVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &m_skyBoxVAO);
    glBindVertexArray(m_skyBoxVAO);

    unsigned int skyVBO;
    glGenBuffers(1, &skyVBO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_pTextureWall = new QOpenGLTexture(QImage(":/images/wall.jpg").mirrored());
    m_pTextureBoard = new QOpenGLTexture(QImage(":/images/board.png").mirrored());

    bool ret = false;
    m_pSkyBoxShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/sky_shader.vert");
    m_pSkyBoxShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/sky_shader.frag");
    ret = m_pSkyBoxShaderProgram.link();
    if (!ret)
        qDebug() << m_pSkyBoxShaderProgram.log();
    m_pSkyBoxShaderProgram.bind();
    m_pSkyBoxShaderProgram.setUniformValue("skybox", 0);
    loadCubeMap();

    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shader.vert");
    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shader.frag");
    ret = m_pShaderProgram.link();
    if (!ret)
        qDebug() << m_pShaderProgram.log();
    m_pShaderProgram.bind();
    //m_pShaderProgram.setUniformValue("texture0", 0);
    m_pShaderProgram.setUniformValue("skybox", 0);

    m_pFramebufShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/framebuf_shader.vert");
    m_pFramebufShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/framebuf_shader.frag");
    ret = m_pFramebufShaderProgram.link();
    if (!ret)
        qDebug() << m_pFramebufShaderProgram.log();
    m_pFramebufShaderProgram.bind();
    m_pFramebufShaderProgram.setUniformValue("texture1", 0);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
        qDebug() << "OpenGL error0: " << error;// 判断是否有错误

    // 绑定帧缓冲对象
    glGenFramebuffers(1, &m_fBufO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fBufO);

    // 设置颜色缓冲附件
    m_pTextureFrameBuf = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_pTextureFrameBuf->create();
    m_pTextureFrameBuf->bind(0);
    m_pTextureFrameBuf->setFormat(QOpenGLTexture::RGBFormat);
    m_pTextureFrameBuf->setSize(width(), height());
    m_pTextureFrameBuf->setMinificationFilter(QOpenGLTexture::Linear);
    m_pTextureFrameBuf->setMagnificationFilter(QOpenGLTexture::Linear);
    m_pTextureFrameBuf->allocateStorage();

    /*glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->width(), this->height(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glBindTexture(GL_TEXTURE_2D, 0);*/

    // 绑定附件在帧缓冲对象
    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pTextureFrameBuf->textureId(), 0);

    // 创建渲染缓冲对象，不用于采样，只用于数据交换，会更快
    glGenRenderbuffers(1, &m_rBuf);
    glBindRenderbuffer(GL_RENDERBUFFER, m_rBuf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width(), height());
    //glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // 绑定附件在帧缓冲对象
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rBuf);

    // 检查帧缓冲是否完整
    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << framebufferStatus;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);// 切换回默认缓冲中
    //m_pTextureGrass = new QOpenGLTexture(QImage(":/images/grass.png"));
   // m_pTextureWin = new QOpenGLTexture(QImage(":/images/blending_transparent_window.png").mirrored());

    //glEnable(GL_DEPTH_TEST);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);// 开启混合
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_FRONT);// 选择剔除哪一个面，背面、正面、正背面GL_BACK、GL_FRONT、GL_FRONT_AND_BACK
    //glFrontFace(GL_CCW);// 定义正逆旋转哪个为正面（GL_CW、GL_CCW）
}

void OpenglWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void OpenglWidget::paintGL()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fBufO);
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bool ret = false;

    // 设置mvp变换矩阵
    QMatrix4x4 model = QMatrix4x4();
    QMatrix4x4 view;
    QMatrix4x4 projection;
    view.lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_up);
    projection.perspective(m_fov, (float)width() / height(), 0.1, 100);

    ret = m_pShaderProgram.bind();
    if (!ret)
        qDebug() << m_pShaderProgram.log();
    m_pShaderProgram.setUniformValue("view", view);
    m_pShaderProgram.setUniformValue("projection", projection);
    m_pShaderProgram.setUniformValue("cameraPos", m_cameraPos);

    model.translate(-1.0f, 0.0f, -1.0f);
    m_pShaderProgram.setUniformValue("model", model);
    glBindVertexArray(m_wVAO);
    //m_pTextureWall->bind(0);
    m_pTexSkyBox->bind(0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    model = QMatrix4x4();
    model.translate(2.0f, 0.0f, 0.0f);
    m_pShaderProgram.setUniformValue("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    m_pTextureWall->release();

    glBindVertexArray(m_bVAO);
    m_pTextureBoard->bind(0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    model = QMatrix4x4();
    m_pShaderProgram.setUniformValue("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    m_pTextureBoard->release();
    glBindVertexArray(0);

    //glDepthMask(GL_FALSE);
    // 最后绘制可以节省一下
    glDepthFunc(GL_LEQUAL);
    m_pSkyBoxShaderProgram.bind();
    m_pSkyBoxShaderProgram.setUniformValue("view", view);
    m_pSkyBoxShaderProgram.setUniformValue("projection", projection);
    glBindVertexArray(m_skyBoxVAO);
    m_pTexSkyBox->bind(0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    m_pTexSkyBox->release();
    glDepthFunc(GL_LESS);
    //glDepthMask(GL_TRUE);

    // 切换到默认帧缓冲
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    glDisable(GL_DEPTH_TEST);
    //glPolygonMode(GL_FRONT, GL_LINE);// 切换为线框模式

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_pFramebufShaderProgram.bind();
    glBindVertexArray(m_qVAO);
    //glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    m_pTextureFrameBuf->bind(0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    m_pTextureFrameBuf->release();
    glBindVertexArray(0);

   /* glBindVertexArray(m_gVAO);
    m_pTextureWin->bind(0);
    // 按照顺序从远到近排序，因为深度测试的原因，先绘制不透明的
    for(std::map<float, QVector3D>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); ++it)
    {
        model = QMatrix4x4();
        model.translate(it->second);
        m_pShaderProgram.setUniformValue("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    m_pTextureWin->release();*/

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

void OpenglWidget::loadCubeMap()
{
    m_pTexSkyBox = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
    m_pTexSkyBox->create();
    m_pTexSkyBox->bind(0);
    m_pTexSkyBox->setSize(m_skyBox[0].width(), m_skyBox[0].height());
    m_pTexSkyBox->setFormat(QOpenGLTexture::RGB8_UNorm);
    m_pTexSkyBox->allocateStorage();

    for (int i = 0; i < m_skyBox.size(); i++)
    {
          QOpenGLTexture::CubeMapFace currentFace = static_cast<QOpenGLTexture::CubeMapFace>(QOpenGLTexture::CubeMapPositiveX + i);
          m_pTexSkyBox->setData(0, 0, currentFace,
                           QOpenGLTexture::RGB, QOpenGLTexture::UInt8,
                           (const void*)m_skyBox[i].constBits(), nullptr);
    }


    // 设置纹理参数
    m_pTexSkyBox->setMagnificationFilter(QOpenGLTexture::Linear);
    m_pTexSkyBox->setMinificationFilter(QOpenGLTexture::Linear);
    m_pTexSkyBox->setWrapMode(QOpenGLTexture::ClampToEdge);
}
