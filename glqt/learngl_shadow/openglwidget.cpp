#include "openglwidget.h"
#include <QOpenGLTexture>
#include <QDateTime>
#include <cmath>
#include <QDebug>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

const QVector3D lightPos(-2.0f, 4.0f, -1.0f);

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

    float planeVertices[] = {
        // positions            // normals         // texcoords
        25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
        -25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
        -25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

        25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
        -25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
        25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
    };

    // 地板数据写入
    glGenVertexArrays(1, &m_planeVAO);
    glBindVertexArray(m_planeVAO);
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof (float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof (float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // 地板纹理
    m_woodTexture = new QOpenGLTexture(QImage(":/images/board.png").mirrored());
    // 立方体纹理
    m_cubeTexture = new QOpenGLTexture(QImage(":/images/container.png").mirrored());

    // 创建帧缓冲，存储深度信息
    glGenFramebuffers(1, &m_fBufO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fBufO);

    // 设置颜色缓冲附件
    m_fDepthTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_fDepthTexture->create();
    m_fDepthTexture->bind(0);
    m_fDepthTexture->setFormat(QOpenGLTexture::DepthFormat);
    m_fDepthTexture->setSize(width(), height());
    m_fDepthTexture->setMinificationFilter(QOpenGLTexture::Linear);
    m_fDepthTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_fDepthTexture->setWrapMode(QOpenGLTexture::Repeat);
    m_fDepthTexture->allocateStorage();
    // 绑定附件在帧缓冲对象
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_fDepthTexture->textureId(), 0);

    // 检查帧缓冲是否完整
    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << framebufferStatus;
    // 切换回默认缓冲中
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shader.vert");
    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shader.frag");
    m_pShaderProgram.link();
    m_pShaderProgram.bind();
    m_pShaderProgram.setUniformValue("depthMap", 0);
    m_pShaderProgram.setUniformValue("diffuseTexture", 1);
    m_pShaderProgram.release();

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
    /* 切线空间与法线贴图：
     * 为什么要存在：在真实的生活中，就算是一面墙各个地方它的法向量也不是完全一样的，因此按一样的法向量渲染出来就不会那么真实；
     * 法线贴图：为了解决这个问题，就在各个图元的法线储存在纹理中，rgb刚好对应xyz，这样会让渲染更加的真实，但是这样又会存在一个问题，法线贴图只能存储一个方向的，因此当物体位置发生变化时，此时法线信息是不正确的；
     * 切线空间：在这样的背景下，为了纹理贴图能够适应各个位置，引出了切线空间TBN，它其实就是一个转换矩阵，将纹理中的法线转换到正确的方向，能够在更个方向通用；
     * 求解TBN：法线向量是本来就有的，只需要求解TB，其实只需要求解任意一个即可，可以通过叉乘得到另外一个
     * 基本原理：因为切线空间的TB轴是对应纹理坐标的UV的，因此可以从纹理坐标推出TB，这是根据矩阵的特性推导的，一个方阵的逆矩阵等于1/行列式*伴随矩阵，因此只需知道三角形的三个顶点坐标以及uv纹理坐标就可以求出TB
     * // positions，基本已知信息
     *  glm::vec3 pos1(-1.0,  1.0, 0.0);
        glm::vec3 pos2(-1.0, -1.0, 0.0);
        glm::vec3 pos3(1.0, -1.0, 0.0);
        glm::vec3 pos4(1.0, 1.0, 0.0);
        // texture coordinates
        glm::vec2 uv1(0.0, 1.0);
        glm::vec2 uv2(0.0, 0.0);
        glm::vec2 uv3(1.0, 0.0);
        glm::vec2 uv4(1.0, 1.0);
        // normal vector
        glm::vec3 nm(0.0, 0.0, 1.0);

        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        GLfloat f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent1 = glm::normalize(tangent1);// 标准化，只需要知道方向

        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent1 = glm::normalize(bitangent1);// 标准化，只需要知道方向
     * 法线贴图的优势：可以在低图元的情况下渲染出高图元的效果，提升性能
     * 缺点：当渲染大量的共用顶点的场景时，为了产生比较柔和的效果会平均化切线向量，导致TBN不再正交，效果会变差，可以通过再次正交来修复
     *  vec3 T = normalize(vec3(model * vec4(tangent, 0.0)));
        vec3 N = normalize(vec3(model * vec4(normal, 0.0)));
        // re-orthogonalize T with respect to N
        T = normalize(T - dot(T, N) * N);
        // then retrieve perpendicular vector B with the cross product of T and N
        vec3 B = cross(T, N);

        mat3 TBN = mat3(T, B, N)
     * 使用方法：
     *  1、因为实在局部空间，需要先转到世界空间，即乘以mode，从法线贴图获取到法向量后直接乘以这个矩阵就转换到世界坐标了
     *  normal = texture(normalMap, fs_in.TexCoords).rgb;
        normal = normalize(normal * 2.0 - 1.0);
        normal = normalize(fs_in.TBN * normal);
        2、先在切线空间完成，即顶点着色器完成，这样会复杂些，但是会节省些性能，因为顶点着色器开销没有那么大
    */
    // 阴影实现第一个阶段：将深度信息缓存到帧缓冲中
    glBindFramebuffer(GL_FRAMEBUFFER, m_fBufO);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 lightProjection, lightView;
    QMatrix4x4 lightSpaceMatrix;
    float near_plane = 1.0f, far_plane = 7.5f;
    lightView.lookAt(lightPos, QVector3D(0.0, 0.0, 0.0), QVector3D(0.0, 1.0, 0.0));
    lightProjection.ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    lightSpaceMatrix = lightProjection * lightView;
    // render scene from light's point of view
    m_pFrameBufShaderProgram.bind();
    m_pFrameBufShaderProgram.setUniformValue("lightSpaceMatrix", lightSpaceMatrix);

    glViewport(0, 0, width(), height());
    glClear(GL_DEPTH_BUFFER_BIT);
    renderScene(m_pFrameBufShaderProgram);

    // 切换回默认缓冲中，阴影实现第二个阶段，正常渲染，利用帧缓冲中的深度信息判断是都处于阴影中来绘制影响
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    // reset viewport
    glViewport(0, 0, width(), height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_pShaderProgram.bind();
    QMatrix4x4 view;
    QMatrix4x4 projection;
    view.lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_up);
    projection.perspective(m_fov, (float)width() / height(), 0.1, 100);

    // 阴影实现第二个阶段，正常渲染，利用帧缓冲中的深度信息判断是都处于阴影中来绘制影响
    m_pShaderProgram.setUniformValue("near_plane", near_plane);
    m_pShaderProgram.setUniformValue("far_plane", far_plane);
    m_pShaderProgram.setUniformValue("view", view);
    m_pShaderProgram.setUniformValue("projection", projection);
    m_pShaderProgram.setUniformValue("lightSpaceMatrix", lightSpaceMatrix);
    m_pShaderProgram.setUniformValue("lightPos", lightPos);
    m_pShaderProgram.setUniformValue("viewPos", m_cameraPos);
    m_fDepthTexture->bind(0);
    renderScene(m_pShaderProgram);
}

void OpenglWidget::renderScene(QOpenGLShaderProgram &shader)
{
    // floor
    QMatrix4x4 model = QMatrix4x4();
    shader.setUniformValue("model", model);
    m_woodTexture->bind(1);
    glBindVertexArray(m_planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    m_woodTexture->release(1);
    // cubes
    model = QMatrix4x4();
    model.translate(QVector3D(0.0f, 1.5f, 0.0));
    model.scale(QVector3D(0.5f, 0.5f, 0.5f));
    shader.setUniformValue("model", model);
    renderCube();
    model = QMatrix4x4();
    model.translate(QVector3D(2.0f, 0.0f, 1.0));
    model.scale(QVector3D(0.5f, 0.5f, 0.5f));
    shader.setUniformValue("model", model);
    renderCube();
    model = QMatrix4x4();
    model.translate(QVector3D(-1.0f, 0.0f, 2.0));
    model.rotate(60.0f, QVector3D(1.0, 0.0, 1.0).normalized());
    model.scale(QVector3D(0.25f, 0.25f, 0.25f));
    shader.setUniformValue("model", model);
    renderCube();
}


// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
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
    m_cubeTexture->bind(1);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    m_cubeTexture->release(1);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void OpenglWidget::renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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

