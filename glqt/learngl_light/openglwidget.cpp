#include "openglwidget.h"
#include <QOpenGLTexture>
#include <QDateTime>
#include <cmath>
#include <QDebug>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

constexpr float PI = 3.1415926f;
QVector3D lightColor(1.0, 1.0, 1.0);
QVector3D objectColor(1.0f, 0.5f, 0.31f);
const QVector3D lightPos(1.2f, 1.0f, 2.0f);
const QVector3D cubePositions[] =
{
  QVector3D( 0.0f,  0.0f,  0.0f),
  QVector3D( 2.0f,  5.0f, -15.0f),
  QVector3D(-1.5f, -2.2f, -2.5f),
  QVector3D(-3.8f, -2.0f, -12.3f),
  QVector3D( 2.4f, -0.4f, -3.5f),
  QVector3D(-1.7f,  3.0f, -7.5f),
  QVector3D( 1.3f, -2.0f, -2.5f),
  QVector3D( 1.5f,  2.0f, -2.5f),
  QVector3D( 1.5f,  0.2f, -1.5f),
  QVector3D(-1.3f,  1.0f, -1.5f)
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

    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    glGenVertexArrays(1, &m_nVAO);
    glBindVertexArray(m_nVAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof (float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof (float)));
    glEnableVertexAttribArray(2);

    glGenVertexArrays(1, &m_nVAOLight);
    glBindVertexArray(m_nVAOLight);// 看绑定区分现在是哪个VAO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);// 用的同一个VBO，不需要重新传数据
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shader.vert");
    m_pShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shader.frag");
    m_pShaderProgram.link();

    m_pLightShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shaderLight.vert");
    m_pLightShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shaderLight.frag");
    m_pLightShaderProgram.link();

    m_texture = new QOpenGLTexture(QImage(":/images/container.png").mirrored());
    m_texture1 = new QOpenGLTexture(QImage(":/images/container_specular.png").mirrored());
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // gamma校正的原因：因为显示器显示出来的颜色亮度不是完全线性的，一般是原来颜色亮度的gamma次幂，gamma约定为2.2
    // 如果想要还原真实的颜色值，有两种方法
    //1、设置为sRGB空间，片段着色器后会自动校正glEnable(GL_FRAMEBUFFER_SRGB);
    //2、自己在片段着色器手动校正
    /*
        void main()
        {
            // do super fancy lighting
            [...]
            // apply gamma correction
            float gamma = 2.2;
            fragColor.rgb = pow(fragColor.rgb, vec3(1.0/gamma));
        }
    */
    //注：一般漫反射都是在sRGB空间中，所以需要避免校正两次，镜面和法线贴图几乎则在线性空间（真实的颜色空间），另外真实空间光源强度一般为距离的2次幂
    // 设置mvp变换矩阵
    QMatrix4x4 model;
    QMatrix4x4 view;
    QMatrix4x4 projection;
    model.rotate(-45, 0.0, 1.0, 0.0);
    view.lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_up);
    projection.perspective(m_fov, (float)width() / height(), 0.1, 100);

    // 绘制橘色立方体
    m_pShaderProgram.bind();
    m_pShaderProgram.setUniformValue("model", model);
    m_pShaderProgram.setUniformValue("view", view);
    m_pShaderProgram.setUniformValue("projection", projection);
    m_pShaderProgram.setUniformValue("objectColor", objectColor);
    m_pShaderProgram.setUniformValue("lightColor", lightColor);
    m_pShaderProgram.setUniformValue("viewPos", m_cameraPos);

    // 设置材质
    //m_pShaderProgram.setUniformValue("material.ambient",  1.0f, 0.5f, 0.31f);
    //m_pShaderProgram.setUniformValue("material.diffuse",  1.0f, 0.5f, 0.31f);
    //m_pShaderProgram.setUniformValue("material.specular", 0.5f, 0.5f, 0.5f);
    m_pShaderProgram.setUniformValue("material.diffuse", 0);
    m_pShaderProgram.setUniformValue("material.specular", 1);
    m_pShaderProgram.setUniformValue("material.shininess", 32.0f);

    // 设置光照的分量强度
    m_pShaderProgram.setUniformValue("light.position",  lightPos);// 点光源
    m_pShaderProgram.setUniformValue("light.lightDirect", 0.0f, 0.0f, -1.0f);// 平行光，类似太阳光
    m_pShaderProgram.setUniformValue("light.ambient",  0.2f, 0.2f, 0.2f);
    m_pShaderProgram.setUniformValue("light.diffuse",  0.5f, 0.5f, 0.5f); // 将光照调暗了一些以搭配场景
    m_pShaderProgram.setUniformValue("light.specular", 1.0f, 1.0f, 1.0f);

    // 设置点光源线性衰减
    m_pShaderProgram.setUniformValue("light.constant",  1.0f);
    m_pShaderProgram.setUniformValue("light.linear",    0.09f);
    m_pShaderProgram.setUniformValue("light.quadratic", 0.032f);

     // 设置聚光
    float cutOff = cos(12.5f * PI / 180);
    float outCutOff = cos(17.5f * PI / 180);
    m_pShaderProgram.setUniformValue("flashlight.position",  m_cameraPos);
    m_pShaderProgram.setUniformValue("flashlight.direction", m_cameraFront);
    m_pShaderProgram.setUniformValue("flashlight.cutOff",   cutOff);
    m_pShaderProgram.setUniformValue("flashlight.outerCutOff",   outCutOff);

    m_texture->bind(0);
    m_texture1->bind(1);
    glBindVertexArray(m_nVAO);
    // 绘制多个对象，就是改变model矩阵，在世界坐标的不同位置
    for(unsigned int i = 0; i < 10; i++)
    {
        QMatrix4x4 model;
        model.translate(cubePositions[i]);
        float angle = 15.0f * i;
        model.rotate(angle, QVector3D(1.0f, 0.3f, 0.5f));
        m_pShaderProgram.setUniformValue("model", model);

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    // 绘制光源立方体
    m_pLightShaderProgram.bind();
    model = QMatrix4x4();
    model.translate(lightPos);
    model.scale(QVector3D(0.2, 0.2, 0.2));
    m_pLightShaderProgram.setUniformValue("model", model);
    m_pLightShaderProgram.setUniformValue("view", view);
    m_pLightShaderProgram.setUniformValue("projection", projection);

    glBindVertexArray(m_nVAOLight);
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
