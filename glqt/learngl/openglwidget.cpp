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

    doneCurrent();
}

void OpenglWidget::initializeGL()
{
    initializeOpenGLFunctions();

    /*float vertices[] =
    {
        0.5f, 0.5f, 0.0f, 1.0f, 1.0f, //右上角 顶点坐标和纹理坐标
        0.5f, -0.5f, 0.0f, 1.0, 0.0, // 右下角
        -0.5f, -0.5f, 0.0f, 0.0, 0.0, // 左下角
        -0.5f, 0.5f, 0.0f, 0.0, 1.0// 左上角
    };*/

    float vertices[] = {
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

    unsigned int indices[] =
    {
        0, 1, 3, // 第一个三角形
        1, 2, 3  // 第二个三角形
    };

    glGenVertexArrays(1, &m_nVAO);
    glBindVertexArray(m_nVAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    unsigned int EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /*unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    m_nshaderProgram = glCreateProgram();
    glAttachShader(m_nshaderProgram, vertexShader);
    glAttachShader(m_nshaderProgram, fragmentShader);
    glLinkProgram(m_nshaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);*/

    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shader.vert");
    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shader.frag");
    m_pShaderProgram.link();

    m_pTextureWall = new QOpenGLTexture(QImage(":/images/wall.jpg").mirrored());
    m_pTextureFace = new QOpenGLTexture(QImage(":/images/awesomeface.png").mirrored());

    m_pShaderProgram.bind();
    m_pShaderProgram.setUniformValue("textureWall", 0);
    m_pShaderProgram.setUniformValue("textureFace", 1);
}

void OpenglWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void OpenglWidget::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 设置mvp变换矩阵
    QMatrix4x4 model;
    QMatrix4x4 view;
    QMatrix4x4 projection;
    model.rotate(-45, 1.0, 0.0, 0.0);

    // 旋转试图
    /*float radius = 10.0f;
    double time = QDateTime::currentDateTime().toSecsSinceEpoch();
    float camX = sin(time) * radius;
    float camZ = cos(time) * radius;
    view.lookAt(QVector3D(camX, 0.0, camZ), m_cameraTarget, m_up);
    //qDebug() << "camX:" << camX;*/

    view.lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_up);
    projection.perspective(m_fov, (float)width() / height(), 0.1, 100);
    m_pShaderProgram.setUniformValue("model", model);
    m_pShaderProgram.setUniformValue("view", view);
    m_pShaderProgram.setUniformValue("projection", projection);

    /*
    //glUseProgram(m_nshaderProgram);
    //glDrawArrays(GL_TRIANGLES, 0, 3);
    // 设置纹理如果超出指定（0，1）的范围后应该如何采样，默认有四种GL_MIRRORED_REPEAT、GL_REPEAT、GL_CLAMP_TO_EDGE、GL_CLAMP_TO_BORDER
    GL_CLAMP_TO_BORDER需要指定边界颜色
    设置如下：
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    */
    // 指定放大或者缩小是如何采样，小纹理、大纹理如何处理,缩小纹理使用多级渐远方式，纹理程序按照距离多分几个
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_pTextureWall->bind(0);
    m_pTextureFace->bind(1);
    // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
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
    // qDebug() << "m_cameraPos:" << m_cameraPos;
}

void OpenglWidget::mouseMoveEvent(QMouseEvent *event)
{
    // 通过鼠标控制模型的移动
    static float pitch = 0;
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
    update();
}

void OpenglWidget::wheelEvent(QWheelEvent *event)
{
    // 滚轮控制模型的大小
    if (m_fov >= 1 && m_fov <= 75)
        m_fov -= event->angleDelta().y() / 120;

    if (m_fov < 1) m_fov = 1;
    if (m_fov > 75) m_fov = 75;
}
