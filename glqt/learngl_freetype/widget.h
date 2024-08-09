#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

class OpenglWidget;

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    OpenglWidget* m_pOpglWidget;
};
#endif // WIDGET_H
