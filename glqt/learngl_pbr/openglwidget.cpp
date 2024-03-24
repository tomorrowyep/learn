#include "openglwidget.h"
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

    m_pbrShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/pbrshader.vert");
    m_pbrShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/pbrshader.frag");
    m_pbrShaderProgram.link();

    m_lightPositions.push_back(QVector3D(-10.0f,  10.0f, 10.0f));
    m_lightPositions.push_back(QVector3D(10.0f,  10.0f, 10.0f));
    m_lightPositions.push_back(QVector3D(-10.0f, -10.0f, 10.0f));
    m_lightPositions.push_back(QVector3D(10.0f, -10.0f, 10.0f));

    m_lightColors.push_back(QVector3D(300.0f, 300.0f, 300.0f));
    m_lightColors.push_back(QVector3D(300.0f, 300.0f, 300.0f));
    m_lightColors.push_back(QVector3D(300.0f, 300.0f, 300.0f));
    m_lightColors.push_back(QVector3D(300.0f, 300.0f, 300.0f));
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

    /*int offset = m_pbrShaderProgram.uniformLocation("lightPositions");
    m_pbrShaderProgram.setUniformValueArray(offset, &m_lightPositions[0], 4);*/
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
        //newPos = m_lightPositions[i];

        QString lightPosUniformName = QString("lightPositions[%1]").arg(i);
        m_pbrShaderProgram.setUniformValue(lightPosUniformName.toStdString().c_str(), newPos);

        model = QMatrix4x4();
        model.translate(newPos);
        model.scale(QVector3D(0.5f, 0.5f, 0.5f));
        m_pbrShaderProgram.setUniformValue("model", model);
        renderSphere();
    }
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

