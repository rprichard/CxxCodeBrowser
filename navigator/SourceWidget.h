#ifndef NAV_SOURCEWIDGET_H
#define NAV_SOURCEWIDGET_H

#include <QPlainTextEdit>

namespace Nav {

class SourceWidget : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit SourceWidget(QWidget *parent = 0);

protected:
    void mousePressEvent(QMouseEvent *event);
};

} // namespace Nav

#endif // NAV_SOURCEWIDGET_H
