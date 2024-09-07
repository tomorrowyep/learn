#include "widget.h"
#include "openglwidget.h"
#include <QVBoxLayout >

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
   this->resize(1000, 800);
   m_pOpglWidget = new OpenglWidget(this);
   QVBoxLayout *vLayout = new QVBoxLayout;
   vLayout->addWidget(m_pOpglWidget);
   this->setLayout(vLayout);
}

Widget::~Widget()
{
}


