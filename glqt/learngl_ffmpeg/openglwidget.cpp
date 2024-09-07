#include "openglwidget.h"
#include "ffmpegparse.h"
#include <QOpenGLTexture>
#include <QDateTime>
#include <cmath>
#include <QDebug>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <thread>
#include <QImage>
#include <QAudioOutput>
#include <QIODevice>
#include <QFile>

namespace
{
constexpr float PI = 3.1415926;
constexpr size_t fontSize = 48;
constexpr size_t sampleSize = 16;// 16位pcm
constexpr int sampleCountPerFrame = 1024;
constexpr const char* fontPath = "E:/software/3dsmax/Composite2014/resources/fonts/Truetype/discb.ttf";
constexpr const wchar_t* videoPath = L"C:/Users/yeteng/Desktop/tools/test/ffmpegtest/res/mp4/oceans.mp4";
constexpr const wchar_t* videoPath02 = (L"C:/Users/yeteng/Desktop/tools/test/ffmpegtest/res/mp4/trailer.mp4");
}


OpenglWidget::OpenglWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    m_cameraRight = QVector3D::crossProduct(m_up, m_cameraFront);

    initFfmpegParse();

    // 设置音频间隔
    int sampleRate = m_ffmpegParse->getSampleRate();
    int interval = sampleCountPerFrame * 1000 / sampleRate;
    m_audioTime =  new QTimer(this);
    m_audioTime->start(interval);
    connect(m_audioTime,  &QTimer::timeout, [this] ()
    {
        // 设置io操作需要异步，不然会阻塞渲染操作
        std::thread (&OpenglWidget::playAudio, this).detach();
    });

     // 设置视频间隔
    int frameRate = m_ffmpegParse->getFrameRate(); // 帧率
    interval = 1000 / frameRate; // 计算间隔
    m_time = new QTimer(this);
    m_time->start(interval);
    connect(m_time, &QTimer::timeout, [this] ()
    {
        update();
    });
}

OpenglWidget::~OpenglWidget()
{
    av_frame_free(&m_rgbFrame);
    av_free(m_pRgbBuffer);
    makeCurrent();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    doneCurrent();
}

void OpenglWidget::initializeGL()
{
    initializeOpenGLFunctions();

    //initCharacterInfos();

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

    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
    projection.ortho(0.0f, width(), 0.0f, height(), 0.f, 100.f);
    // 错误，需要指定近平面和远平面的距离，2d图形z为0，所以近平面只能为0才能渲染出来，不然被裁剪掉了
   // projection.ortho(QRectF(0.0f, width(), 0.0f, height()));

    m_pShaderProgram.setUniformValue("projection", projection);
    m_pShaderProgram.setUniformValue("image", 0);

     glBindVertexArray(m_nVAO);
    // 指定放大或者缩小是如何采样，小纹理、大纹理如何处理,缩小纹理使用多级渐远方式，纹理程序按照距离多分几个
    /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    renderText(m_pShaderProgram, "This is sample text", 25.0f, 25.0f, 1.0f, QVector3D(0.5, 0.8f, 0.2f));
    renderText(m_pShaderProgram, "(C) LearnOpenGL.com", 510.0f, 570.0f, 0.5f, QVector3D(0.3, 0.7f, 0.9f));*/

    renderVideo();
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

void OpenglWidget::initFfmpegParse()
{
    m_rgbFrame = av_frame_alloc();
    m_ffmpegParse = std::make_unique<FfmpegParse>(videoPath02);
    if (!m_ffmpegParse->openLocalFile())
        return;

    // 初始化音视频解码器
    m_ffmpegParse->initVideoInfo();

    // 设置音频输出到声卡的格式
    m_fmt.setSampleRate(m_ffmpegParse->getSampleRate());
    m_fmt.setSampleSize(sampleSize); // 设置样本大小
    m_fmt.setChannelCount(m_ffmpegParse->getchannels());
    m_fmt.setCodec("audio/pcm");
    m_fmt.setByteOrder(QAudioFormat::LittleEndian);
    m_fmt.setSampleType(QAudioFormat::SignedInt);

    // 初始化 QAudioOutput 对象
    m_audioOut = new QAudioOutput(m_fmt);
    m_audioDevice = m_audioOut->start(); // 开始音频输出

    // 多线程解析音频
    /*std::thread audioFrameHandl([&](){
        m_ffmpegParse->readAudio();
    });
    audioFrameHandl.detach();*/

   // 多线程解析视频
    std::thread vedioFrameHandl([&](){
        m_ffmpegParse->readFrame();
    });
    vedioFrameHandl.detach();
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

void OpenglWidget::renderVideo()
{
    // 从缓存中获取图像
    auto image = yuv2RGB(ImagesCache::instance().getYUVFrame());
    if (image.isNull())
        return;

    // 创建并绑定纹理
    if (m_pTextureVideo)
    {
        delete m_pTextureVideo; // 释放之前的纹理
        m_pTextureVideo = nullptr;
    }

    m_pTextureVideo = new QOpenGLTexture(image.mirrored());
    m_pTextureVideo->setMinificationFilter(QOpenGLTexture::Linear);
    m_pTextureVideo->setMagnificationFilter(QOpenGLTexture::Linear);
    m_pTextureVideo->setWrapMode(QOpenGLTexture::ClampToEdge);
    m_pTextureVideo->bind(0);

    GLfloat leftX = (width() / 2.0) - (image.width() / 2.0);
    GLfloat topY = (height() / 2.0) + (image.height() / 2.0);

    GLfloat vertices[] =
    {
        leftX, topY,  0.0, 1.0,
        leftX + image.width(), topY, 1.0, 1.0,
        leftX, topY - image.height(),  0.0, 0.0,

        leftX, topY - image.height(), 0.0, 0.0,
        leftX + image.width(), topY - image.height(), 1.0, 0.0,
        leftX + image.width(), topY, 1.0, 1.0
    };

    // 绑定缓冲区并更新数据
    glBindBuffer(GL_ARRAY_BUFFER, m_nVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    auto error = glGetError();
    if (error != GL_NO_ERROR) {
        qDebug() << "OpenGL Error: " << error;
    }

    // 绘制图像
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void OpenglWidget::playAudio()
{
    // 设置音频
    auto info = ImagesCache::instance().getAudioFrame();
    if (!cover2Qt(info))
        return;

    // 将缓冲区数据写入 QIODevice 以供播放
    if (m_audioDevice)
        m_audioDevice->write((char*)m_audioBuffer.get(), m_bufferSize);
}

QImage OpenglWidget::yuv2RGB(ImagesCache::VideoInfo info)
{
    if (!m_pRgbBuffer)
        allocateRgbBuf(info);

    if (!info.frame)
        return QImage();

    // 将YUV转换为RGB
    sws_scale(info.swc,
              info.frame->data,
              info.frame->linesize,
              0,
              info.vedioHeight,
              m_rgbFrame->data,
              m_rgbFrame->linesize);

    // 删除数据
    av_frame_free(&info.frame);

    // 将RGB数据转为QImage并显示
    return QImage (m_rgbFrame->data[0],
            info.vedioWight,
            info.vedioHeight,
            m_rgbFrame->linesize[0],
            QImage::Format_RGB888);
}

bool OpenglWidget::allocateRgbBuf(const ImagesCache::VideoInfo &info)
{
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, info.vedioWight, info.vedioHeight, 32);
    if (bufferSize < 0)
        return false;

    // 分配合适大小的空间
    m_pRgbBuffer = static_cast<uint8_t*>(av_malloc(bufferSize));
    if (m_pRgbBuffer == nullptr)
        return false;

    // 将缓冲区数据填充到 data 和 linesize 数组中，初始化m_pFrameRGB
    int ret = av_image_fill_arrays(m_rgbFrame->data, m_rgbFrame->linesize, m_pRgbBuffer, AV_PIX_FMT_RGB24, info.vedioWight, info.vedioHeight, 32);
    if (ret < 0)
        return false;

    return true;
}

bool OpenglWidget::cover2Qt(ImagesCache::AudioInfo &info)
{
    if (!info.frame || !info.frame->data[0])
        return false;

    if (!m_audioBuffer)
    {
         m_bufferSize = av_samples_get_buffer_size(nullptr, m_ffmpegParse->getchannels(), info.frame->nb_samples, AV_SAMPLE_FMT_S16, 1);
         m_audioBuffer = std::make_unique<uint8_t[]>(m_bufferSize); // 为缓冲区分配内存
    }

    auto buffer = m_audioBuffer.get();
    int samplecount = swr_get_out_samples(info.swr, info.frame->nb_samples);
    int ret = swr_convert(
        info.swr,
        &buffer, // 转换后的数据
        samplecount, // 输出缓冲区的大小
        (const uint8_t **)info.frame->data, // 输入数据
        info.frame->nb_samples // 输入样本数量
    );

    if (ret < 0)
        return false;

    av_frame_free(&(info.frame));  // 删除数据

    return true;
}

