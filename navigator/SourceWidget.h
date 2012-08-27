#ifndef NAV_SOURCEWIDGET_H
#define NAV_SOURCEWIDGET_H

#include <QPlainTextEdit>

namespace Nav {

class File;

class SourceWidget : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit SourceWidget(QWidget *parent = 0);
    void setFile(File *file);
    File *file() { return m_file; }
    void selectIdentifier(int line, int column);

private slots:
    void actionCrossReferences();

private:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void contextMenuEvent(QContextMenuEvent *e);
    void clearSelection();
    QTextCursor findEnclosingIdentifier(QTextCursor pt);

    File *m_file;
};

} // namespace Nav

#endif // NAV_SOURCEWIDGET_H
