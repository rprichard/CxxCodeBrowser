#ifndef NAVSOURCEWIDGET_H
#define NAVSOURCEWIDGET_H

#include <QPlainTextEdit>

class NavSourceWidget : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit NavSourceWidget(QWidget *parent = 0);
    
protected:
    void mousePressEvent(QMouseEvent *event);
};

#endif // NAVSOURCEWIDGET_H
