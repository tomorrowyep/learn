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
constexpr size_t fontSize = 48;
constexpr const char* fontPath = "E:/software/3dsmax/Composite2014/resources/fonts/Truetype/discb.ttf";

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

    initCharacterInfos();

    glGenVertexArrays(1, &m_nVAO);
    glBindVertexArray(m_nVAO);

    glGenBuffers(1, &m_nVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_nVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void OpenglWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void OpenglWidget::paintGL()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_pShaderProgram.bind();
    QMatrix4x4 projection;
    projection.ortho(0.0f, width(), 0.0f, height(), 0.f, 0.1f);
    // 错误，需要指定近平面和远平面的距离，2d图形z为0，所以近平面只能为0才能渲染出来，不然被裁剪掉了
   // projection.ortho(QRectF(0.0f, width(), 0.0f, height()));

    m_pShaderProgram.setUniformValue("projection", projection);

    // 指定放大或者缩小是如何采样，小纹理、大纹理如何处理,缩小纹理使用多级渐远方式，纹理程序按照距离多分几个
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    renderText(m_pShaderProgram, "This is sample text", 25.0f, 25.0f, 1.0f, QVector3D(0.5, 0.8f, 0.2f));
    renderText(m_pShaderProgram, "(C) LearnOpenGL.com", 510.0f, 570.0f, 0.5f, QVector3D(0.3, 0.7f, 0.9f));
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
    return;
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

void OpenglWidget::initCharacterInfos()
{
    m_ftManager = std::make_unique<FreeTypeManager>();

    m_ftManager->loadFontFace(fontPath);
    m_ftManager->setFontSize(fontSize);

    FreeTypeManager::CharacterInfo info;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); //禁用字节对齐限制
    for (GLubyte character = 0; character < 128; ++character)
    {
        info  = {}; // 清除数据
        // 获取字符信息并存入缓存
        m_ftManager->getCharInfo(info, character);

        // 生成纹理
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RED,
                    info.imageSize.width(),
                    info.imageSize.height(),
                    0,
                    GL_RED,
                    GL_UNSIGNED_BYTE,
                    info.imageData
                    );

        info.textureId = texture;
        m_characterInfos.insert(std::make_pair(character, info));

        // 设置纹理选项
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
}

void OpenglWidget::renderText(QOpenGLShaderProgram &shader, const std::string text, GLfloat startX, GLfloat startY, const GLfloat scale, const QVector3D color)
{
    shader.bind();
    m_pShaderProgram.setUniformValue("textColor", color);
    glBindVertexArray(m_nVAO);

    // 遍历文本中所有的字符
    std::string::const_iterator characterIter;
    for (characterIter = text.begin(); characterIter != text.end(); ++characterIter)
    {
        FreeTypeManager::CharacterInfo character = m_characterInfos[*characterIter];

        // 获取字符左下角的位置
        GLfloat xpos = startX + character.bearing.x() * scale;
        GLfloat ypos = startY - (character.imageSize.height() - character.bearing.y()) * scale;

        GLfloat w = character.imageSize.width() * scale;
        GLfloat h = character.imageSize.height() * scale;

        // 对每个字符更新VBO
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }
        };

        // 在四边形上绘制字形纹理
        glBindTexture(GL_TEXTURE_2D, character.textureId);

        // 更新VBO内存的内容
        glBindBuffer(GL_ARRAY_BUFFER, m_nVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // 渲染字符
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 更新位置到下一个字形的原点，注意单位是1/64像素
        startX += (character.advance >> 6) * scale; // 位偏移6个单位来获取单位为像素的值 (2^6 = 64)
    }
}
