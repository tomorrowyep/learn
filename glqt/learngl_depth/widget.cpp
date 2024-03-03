#include "widget.h"
#include "openglwidget.h"
#include <QVBoxLayout >

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    this->resize(800, 700);

    QSurfaceFormat format;
    format.setSamples(4); // 设置MSAA样本数量
    QSurfaceFormat::setDefaultFormat(format);

    m_pOpglWidget = new OpenglWidget(this);
    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(m_pOpglWidget);
    this->setLayout(vLayout);
}

Widget::~Widget()
{
}


