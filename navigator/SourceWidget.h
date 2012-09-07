#ifndef NAV_SOURCEWIDGET_H
#define NAV_SOURCEWIDGET_H

#include <QList>
#include <QPlainTextEdit>
#include <QScrollArea>

#include "CXXSyntaxHighlighter.h"

namespace Nav {

class File;
class Project;

class SourceWidgetView : public QWidget
{
    Q_OBJECT
public:
    SourceWidgetView(Project &project, File &file);
    File &file() { return m_file; }

protected:
    void paintEvent(QPaintEvent *event);
    QSize sizeHint() const;

private:
    Project &m_project;
    File &m_file;
    QList<std::pair<int, int> > m_linePosition;
    QVector<CXXSyntaxHighlighter::Kind> m_syntaxColoring;
    int m_maxLineCount;
};

class SourceWidget : public QScrollArea
{
    Q_OBJECT
public:
    explicit SourceWidget(Project &project, File &file, QWidget *parent = 0);
    void setFile(File &file);
    File &file() { return sourceWidgetView().file(); }

private:
    SourceWidgetView &sourceWidgetView();

public:
    void selectIdentifier(int line, int column);

#if 0
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
#endif

    Project &m_project;
};

} // namespace Nav

#endif // NAV_SOURCEWIDGET_H
