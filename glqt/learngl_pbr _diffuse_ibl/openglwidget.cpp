#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "openglwidget.h"
#include <string>
#include <QOpenGLTexture>
#include <QDateTime>
#include <cmath>
#include <QDebug>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

constexpr const int nrRows    = 7;
constexpr const int nrColumns = 7;
constexpr const int cubeMapSize = 512;
constexpr const int irradianceSize = 32;
constexpr const float spacing = 2.5;

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

    // 加载hdr数据，使用stb库
    stbi_set_flip_vertically_on_load(true); // 设置为 true，加载时进行垂直翻转，y方向上的数据是相反的
    int width, height, nrComponents;
    auto data = stbi_loadf(std::string("E:/yt/data/opgl/glqt/learngl_pbr _diffuse_ibl/cloudy.hdr").c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        glGenTextures(1, &m_hdrTexture);
        glBindTexture(GL_TEXTURE_2D, m_hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        qDebug() << "Failed to load HDR image.";
    }

    m_pbrShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/pbrshader.vert");
    m_pbrShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/pbrshader.frag");
    m_pbrShaderProgram.link();

    m_cubeMapShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/cubemapshader.vert");
    m_cubeMapShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/cubemapshader.frag");
    m_cubeMapShaderProgram.link();

    m_irradianceShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/irradianceshader.vert");
    m_irradianceShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/irradianceshader.frag");
    m_irradianceShaderProgram.link();

    m_backgroundShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/backgroundshader.vert");
    m_backgroundShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/backgroundshader.frag");
    m_backgroundShaderProgram.link();

    m_lightPositions.push_back(QVector3D(-10.0f,  10.0f, 10.0f));
    m_lightPositions.push_back(QVector3D(10.0f,  10.0f, 10.0f));
    m_lightPositions.push_back(QVector3D(-10.0f, -10.0f, 10.0f));
    m_lightPositions.push_back(QVector3D(10.0f, -10.0f, 10.0f));

    m_lightColors.push_back(QVector3D(300.0f, 300.0f, 300.0f));
    m_lightColors.push_back(QVector3D(300.0f, 300.0f, 300.0f));
    m_lightColors.push_back(QVector3D(300.0f, 300.0f, 300.0f));
    m_lightColors.push_back(QVector3D(300.0f, 300.0f, 300.0f));

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    // 创建帧缓冲，存储颜色信息
    glGenFramebuffers(1, &m_fCubeMapBuf);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fCubeMapBuf);

    // 绑定渲染缓冲附件
    unsigned int captureRBO;
    glGenRenderbuffers(1, &captureRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cubeMapSize, cubeMapSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    // 设置颜色缓冲附件
    loadCubeMap(m_fCubeMapTexture, cubeMapSize);
    //glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_fCubeMapTexture->textureId(), 0);

    // 设置立方体贴图每个面的vp矩阵
    QMatrix4x4 cubeMapProjection;
    cubeMapProjection.perspective(90.0f, 1.0f, 0.1f, 10.0f);

    QMatrix4x4 tmp;
    tmp.lookAt(QVector3D(0.0,0.0,0.0), QVector3D(1.0,0.0,0.0), QVector3D(0.0,-1.0,0.0));
    m_cubeMapViews.push_back(tmp);
    tmp = QMatrix4x4();
    tmp.lookAt(QVector3D(0.0,0.0,0.0), QVector3D(-1.0,0.0,0.0), QVector3D(0.0,-1.0,0.0));
    m_cubeMapViews.push_back(tmp);
    tmp = QMatrix4x4();
    tmp.lookAt(QVector3D(0.0,0.0,0.0), QVector3D(0.0,1.0,0.0), QVector3D(0.0,0.0,1.0));
    m_cubeMapViews.push_back(tmp);
    tmp = QMatrix4x4();
    tmp.lookAt(QVector3D(0.0,0.0,0.0), QVector3D(0.0,-1.0,0.0), QVector3D(0.0,0.0,-1.0));
    m_cubeMapViews.push_back( tmp);
    tmp = QMatrix4x4();
    tmp.lookAt(QVector3D(0.0,0.0,0.0), QVector3D(0.0,0.0,1.0), QVector3D(0.0,-1.0,0.0));
    m_cubeMapViews.push_back(tmp);
    tmp = QMatrix4x4();
    tmp.lookAt(QVector3D(0.0,0.0,0.0), QVector3D(0.0,0.0,-1.0), QVector3D(0.0,-1.0,0.0));
    m_cubeMapViews.push_back(tmp);

    m_cubeMapShaderProgram.bind();
    m_cubeMapShaderProgram.setUniformValue("equirectangularMap", 0);
    m_cubeMapShaderProgram.setUniformValue("projection", cubeMapProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_hdrTexture);

    glViewport(0, 0, cubeMapSize, cubeMapSize);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_cubeMapShaderProgram.setUniformValue("view", m_cubeMapViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_fCubeMapTexture->textureId(), 0);

        renderCube(); // renders a 1x1 cube
    }

    // 检查帧缓冲是否完整
    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << framebufferStatus;

    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    // 创建辐照度帧缓冲，存储辐照度信息
    glGenFramebuffers(1, &m_irradianceBuf);
    glBindFramebuffer(GL_FRAMEBUFFER, m_irradianceBuf);

    // 绑定渲染缓冲附件
    unsigned int irradianceRBO;
    glGenRenderbuffers(1, &irradianceRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, irradianceRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, irradianceSize, irradianceSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, irradianceRBO);

    // 设置颜色缓冲附件
    loadCubeMap(m_irradianceMapTexture, irradianceSize);

    m_irradianceShaderProgram.bind();
    m_irradianceShaderProgram.setUniformValue("projection", cubeMapProjection);
    m_fCubeMapTexture->bind();

    glViewport(0, 0, irradianceSize, irradianceSize);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_irradianceShaderProgram.setUniformValue("view", m_cubeMapViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_irradianceMapTexture->textureId(), 0);

        renderCube(); // renders a 1x1 cube
    }

    // 检查帧缓冲是否完整
    framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << framebufferStatus;

    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
}

void OpenglWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void OpenglWidget::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width(), height());

    QMatrix4x4 view;
    QMatrix4x4 projection;
    view.lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_up);
    projection.perspective(m_fov, (float)width() / height(), 0.1, 100);
    m_pbrShaderProgram.bind();
    m_pbrShaderProgram.setUniformValue("view", view);
    m_pbrShaderProgram.setUniformValue("projection", projection);

    m_pbrShaderProgram.setUniformValue("camPos", m_cameraPos);
    m_pbrShaderProgram.setUniformValue("albedo", QVector3D(0.5f, 0.0f, 0.0f));
    m_pbrShaderProgram.setUniformValue("ao", 1.0f);

    m_irradianceMapTexture->bind();
    // 绘制49个球体
    auto model = QMatrix4x4();
    for (int row = 0; row < nrRows; ++row)
    {
        m_pbrShaderProgram.setUniformValue("metallic", (float)row / (float)nrRows);
        for (int col = 0; col < nrColumns; ++col)
        {
            // we clamp the roughness to 0.05 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit off
            // on direct lighting.
            m_pbrShaderProgram.setUniformValue("roughness", qBound(0.05f, static_cast<float>(col) / static_cast<float>(nrColumns), 1.0f));
            model = QMatrix4x4();
            model.translate(QVector3D(
                                       (col - (nrColumns / 2)) * spacing,
                                       (row - (nrRows / 2)) * spacing,
                                       0.0f
                                       ));
            m_pbrShaderProgram.setUniformValue("model", model);
            renderSphere();
        }
    }

    int offset = m_pbrShaderProgram.uniformLocation("lightColors");
    m_pbrShaderProgram.setUniformValueArray(offset, &m_lightColors[0], 4);

    // 绘制球形光源
    for (unsigned int i = 0; i < m_lightPositions.size(); ++i)
    {
        // 在Qt中获取当前时间
        QTime currentTime = QTime::currentTime();
        // 计算sin(glfwGetTime() * 5.0)
        float sinValue = sin(currentTime.msecsSinceStartOfDay() / 1000.0f * 5.0f);
        // 计算新的位置
        QVector3D newPos = m_lightPositions[i] + QVector3D(sinValue * 5.0f, 0.0f, 0.0f);
        newPos = m_lightPositions[i];

        QString lightPosUniformName = QString("lightPositions[%1]").arg(i);
        m_pbrShaderProgram.setUniformValue(lightPosUniformName.toStdString().c_str(), newPos);

        model = QMatrix4x4();
        model.translate(newPos);
        model.scale(QVector3D(0.5f, 0.5f, 0.5f));
        m_pbrShaderProgram.setUniformValue("model", model);
        renderSphere();
    }

    // 绘制立方体贴图
    m_backgroundShaderProgram.bind();
    m_backgroundShaderProgram.setUniformValue("view", view);
    m_backgroundShaderProgram.setUniformValue("projection", projection);
    m_fCubeMapTexture->bind();
    //m_irradianceMapTexture->bind();
    renderCube();
}

void OpenglWidget::loadCubeMap(QOpenGLTexture* &texture, int size)
{
    texture = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
    texture->create();
    texture->bind();
    texture->setSize(size, size);
    texture->setFormat(QOpenGLTexture::RGB16F);
    texture->allocateStorage();

    std::vector<QVector3D> colorData(size * size, QVector3D(0.0f, 0.0f, 0.0f));
    for (int i = 0; i < 6; i++)
    {
        QOpenGLTexture::CubeMapFace currentFace = static_cast<QOpenGLTexture::CubeMapFace>(QOpenGLTexture::CubeMapPositiveX + i);
        texture->setData(0, 0, currentFace,
                         QOpenGLTexture::RGB, QOpenGLTexture::Float32,
                         (const void*)colorData.data(), nullptr);
    }

    // 设置纹理的一些表现形式
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    texture->setMinificationFilter(QOpenGLTexture::Linear);
    texture->setWrapMode(QOpenGLTexture::ClampToEdge);
}

// 绘制一个球体
unsigned int sphereVAO = 0;
unsigned int indexCount;
void  OpenglWidget::renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<QVector3D> positions;
        std::vector<QVector3D> uv;
        std::vector<QVector3D> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(QVector3D(xPos, yPos, zPos));
                uv.push_back(QVector2D(xSegment, ySegment));
                normals.push_back(QVector3D(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y       * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y       * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = static_cast<unsigned int>(indices.size());

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x());
            data.push_back(positions[i].y());
            data.push_back(positions[i].z());
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x());
                data.push_back(normals[i].y());
                data.push_back(normals[i].z());
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x());
                data.push_back(uv[i].y());
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void OpenglWidget::renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
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

