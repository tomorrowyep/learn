#include "openglwidget.h"
#include <QOpenGLTexture>
#include <QDateTime>
#include <cmath>
#include <random>
#include <QDebug>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

namespace
{
    std::vector<QVector3D> objectPositions
    {
        QVector3D(-3.0, -3.0, -3.0),
                QVector3D(0.0, -3.0, -3.0),
                QVector3D(3.0, -3.0, -3.0),
                QVector3D(-3.0, -3.0, 0.0),
                QVector3D(0.0, -3.0, 0.0),
                QVector3D(3.0, -3.0, 0.0),
                QVector3D(-3.0, -3.0, 3.0),
                QVector3D(0.0, -3.0, 3.0),
                QVector3D(3.0, -3.0, 3.0)
    };
    const GLuint NR_LIGHTS = 32;

    GLfloat lerp(GLfloat a, GLfloat b, GLfloat f)
    {
        return a + f * (b - a);
    }
}

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

     // - Colors
     srand(13);
     for (GLuint i = 0; i < NR_LIGHTS; i++)
     {
         // Calculate slightly random offsets
         GLfloat xPos = ((rand() % 100) / 100.0) * 6.0 - 3.0;
         GLfloat yPos = ((rand() % 100) / 100.0) * 6.0 - 4.0;
         GLfloat zPos = ((rand() % 100) / 100.0) * 6.0 - 3.0;
         m_lightPositions.push_back(QVector3D(xPos, yPos, zPos));
         // Also calculate random color
         GLfloat rColor = ((rand() % 100) / 200.0f) + 0.5; // Between 0.5 and 1.0
         GLfloat gColor = ((rand() % 100) / 200.0f) + 0.5; // Between 0.5 and 1.0
         GLfloat bColor = ((rand() % 100) / 200.0f) + 0.5; // Between 0.5 and 1.0
         m_lightColors.push_back(QVector3D(rColor, gColor, bColor));
     }

    // 立方体纹理
    m_woodTexture = new QOpenGLTexture(QImage(":/images/container.png").mirrored());

    // 创建SSAO的Gbuf，存储颜色计算需要的信息
    glGenFramebuffers(1, &m_fGBufO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fGBufO);

    // 设置颜色缓冲附件并绑定附件在帧缓冲对象
    // 设置location，渲染到对应的颜色缓冲附件
    GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    createAttatch(m_posTexture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_posTexture->textureId(), 0);

    createAttatch(m_norTexture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 1, GL_TEXTURE_2D, m_norTexture->textureId(), 0);

    createAttatch(m_diffuseAndSpecularTexture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 2, GL_TEXTURE_2D, m_diffuseAndSpecularTexture->textureId(), 0);

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

    // 创建SSAO缓冲区，存储遮蔽因子
    glGenFramebuffers(1, &m_fSSAOBuf);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fSSAOBuf);

    createAttatch(m_SSAOTexture, true);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SSAOTexture->textureId(), 0);
    // 检查帧缓冲是否完整
    framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << framebufferStatus;


    // 创建SSAO模糊缓冲区，存储模糊后的数据
    glGenFramebuffers(1, &m_fSSAOBlurBuf);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fSSAOBlurBuf);

    createAttatch(m_SSAOBlurTexture, true);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SSAOBlurTexture->textureId(), 0);
    // 检查帧缓冲是否完整
    framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << framebufferStatus;

    // 采样核心
    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    for (GLuint i = 0; i < 64; ++i)
    {
        QVector3D sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample.normalize();
        sample *= randomFloats(generator);
        GLfloat scale = GLfloat(i) / 64.0;

        // Scale samples s.t. they're more aligned to center of kernel
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        m_ssaoKernel.push_back(sample);
    }

    // 噪声纹理
    std::vector<QVector3D> ssaoNoise;
    for (GLuint i = 0; i < 16; i++)
    {
        QVector3D noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
        ssaoNoise.push_back(noise);
    }
    glGenTextures(1, &m_noiseTexture);
    glBindTexture(GL_TEXTURE_2D, m_noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    m_pFrameBufShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/framebufshader.vert");
    m_pFrameBufShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/framebufshader.frag");
    m_pFrameBufShaderProgram.link();

    m_pSSAOShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/ssaoshader.vert");
    m_pSSAOShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/ssaoshader.frag");
    m_pSSAOShader.link();
    m_pSSAOShader.bind();
    m_pSSAOShader.setUniformValue("gPositionDepth", 0);
    m_pSSAOShader.setUniformValue("gNormal", 1);
    m_pSSAOShader.setUniformValue("texNoise", 3);

    m_pSSAOBlurShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/ssaoblurshader.vert");
    m_pSSAOBlurShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/ssaoblurshader.frag");
    m_pSSAOBlurShader.link();

    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shader.vert");
    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shader.frag");
    m_pShaderProgram.link();
    m_pShaderProgram.bind();
    m_pShaderProgram.setUniformValue("gPosition", 0);
    m_pShaderProgram.setUniformValue("gNormal", 1);
    m_pShaderProgram.setUniformValue("gAlbedoSpec", 2);
    m_pShaderProgram.setUniformValue("ssao", 3);

    m_pShaderLight.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/lightshader.vert");
    m_pShaderLight.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/lightshader.frag");
    m_pShaderLight.link();

    glEnable(GL_DEPTH_TEST);
}

void OpenglWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void OpenglWidget::paintGL()
{
    // 第一阶段，正常渲染，获取到相关的一系列数据
    glBindFramebuffer(GL_FRAMEBUFFER, m_fGBufO);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 view;
    QMatrix4x4 projection;
    view.lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_up);
    projection.perspective(m_fov, (float)width() / height(), 0.1, 100);
    m_pFrameBufShaderProgram.bind();
    m_pFrameBufShaderProgram.setUniformValue("view", view);
    m_pFrameBufShaderProgram.setUniformValue("projection", projection);
    m_woodTexture->bind(0);

    // 绘制多个立方体
    auto model = QMatrix4x4();
    for (GLuint i = 0; i < objectPositions.size(); i++)
    {
        model = QMatrix4x4();
        model.translate(objectPositions[i]);
        //model.scale(QVector3D(0.5f, 0.5f, 0.5f));
        m_pFrameBufShaderProgram.setUniformValue("model", model);
        renderCube();
    }
     m_woodTexture->release(0);

     // 第二阶段 SSAO计算遮蔽因子，应用采样核心
     glBindFramebuffer(GL_FRAMEBUFFER, m_fSSAOBuf);
     m_pSSAOShader.bind();
     m_posTexture->bind(0);
     m_norTexture->bind(1);
     glActiveTexture(GL_TEXTURE2);
     glBindTexture(GL_TEXTURE_2D, m_noiseTexture);
     int offset = m_pSSAOShader.uniformLocation("samples");
     m_pSSAOShader.setAttributeArray(offset, &m_ssaoKernel[0], 64);
     offset = m_pSSAOShader.uniformLocation("projection");
     m_pSSAOShader.setAttributeArray(offset, projection.constData(), 16);
     renderQuad();

     // 第三阶段 模糊遮蔽因子，根据噪声纹理
     glBindFramebuffer(GL_FRAMEBUFFER, m_fSSAOBlurBuf);
     m_pSSAOBlurShader.bind();
     m_SSAOTexture->bind();
     renderQuad();

     // 第四阶段，光照计算
     glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

     m_pShaderProgram.bind();
     m_posTexture->bind(0);
     m_norTexture->bind(1);
     m_diffuseAndSpecularTexture->bind(2);
     m_SSAOBlurTexture->bind(3);

    for (GLuint i = 0; i < m_lightPositions.size(); i++)
    {
        QString positionUniformName = QString("lights[%1].Position").arg(i);
        QString colorUniformName = QString("lights[%1].Color").arg(i);
        QString linearUniformName = QString("lights[%1].Linear").arg(i);
        QString quadraticUniformName = QString("lights[%1].Quadratic").arg(i);
        QString radiusUniformName = QString("lights[%1].Radius").arg(i);

        m_pShaderProgram.setUniformValue(positionUniformName.toStdString().c_str(), m_lightPositions[i]);
        m_pShaderProgram.setUniformValue(colorUniformName.toStdString().c_str(), m_lightColors[i]);

        // Update attenuation parameters and calculate radius
        const GLfloat constant = 1.0;
        const GLfloat linear = 0.7;
        const GLfloat quadratic = 1.8;
        m_pShaderProgram.setUniformValue(linearUniformName.toStdString().c_str(), linear);
        m_pShaderProgram.setUniformValue(quadraticUniformName.toStdString().c_str(), quadratic);

        // Then calculate radius of light volume/sphere
        /*但是在真正的使用当中，如果有大量的光源，这样的方法计算光照还是比较繁琐，因此这里一般采用光体积的方法，就是计算出光源的影响半径，在半径内才进行光照
         ，这样可以避免无用光照，只会计算有影响的光照*/
        const GLfloat maxBrightness = std::fmaxf(std::fmaxf(m_lightColors[i].x(), m_lightColors[i].y()), m_lightColors[i].z());
        GLfloat radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0 / 5.0) * maxBrightness))) / (2 * quadratic);// 计算公式就是二次函数的求根方程，左边的值为5/256为黑暗的光照强度，因为二次函数不可能为零
        m_pShaderProgram.setUniformValue(radiusUniformName.toStdString().c_str(), radius);
    }
    m_pShaderProgram.setUniformValue("viewPos", m_cameraPos);
    renderQuad();

    // 将帧缓冲的深度信息复制到默认缓冲中，这样才能够正确的渲染场景
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fGBufO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFramebufferObject()); // Write to default framebuffer
    glBlitFramebuffer(0, 0, width(), height(), 0, 0, width(), height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    // 第五阶段 结合正向渲染和延迟渲染，正向渲染所有光立方体
    m_pShaderLight.bind();
    m_pShaderLight.setUniformValue("view", view);
    m_pShaderLight.setUniformValue("projection", projection);
    for (GLuint i = 0; i < m_lightPositions.size(); i++)
    {
        model = QMatrix4x4();
        model.translate(m_lightPositions[i]);
        model.scale(QVector3D(0.25f, 0.25f, 0.25f));
        m_pShaderLight.setUniformValue("model", model);
        m_pShaderLight.setUniformValue("lightColor", m_lightColors[i]);
        renderCube();
    }
}

void OpenglWidget::createAttatch(QOpenGLTexture *&texture, bool isSignalComponent)
{
    texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    texture->create();
    texture->bind();
    if (isSignalComponent)
        texture->setFormat(QOpenGLTexture::R16F);
    else
        texture->setFormat(QOpenGLTexture::RGBA16F);
    texture->setSize(width(), height());
    texture->setMinificationFilter(QOpenGLTexture::Linear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    texture->setWrapMode(QOpenGLTexture::ClampToEdge);
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

