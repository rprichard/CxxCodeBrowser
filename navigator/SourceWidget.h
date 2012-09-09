#ifndef NAV_SOURCEWIDGET_H
#define NAV_SOURCEWIDGET_H

#include <QList>
#include <QPlainTextEdit>
#include <QScrollArea>

#include "CXXSyntaxHighlighter.h"

namespace Nav {

class File;
class Project;
class SourceWidget;
class SourceWidgetLineArea;


///////////////////////////////////////////////////////////////////////////////
// SourceWidgetLineArea

// The line number area to the left of the source widget.
class SourceWidgetLineArea : public QWidget
{
    Q_OBJECT
public:
    SourceWidgetLineArea(const QMargins &margins, QWidget *parent = 0) :
        QWidget(parent), m_margins(margins), m_lineCount(0)
    {
    }

    QSize sizeHint() const;
    void paintEvent(QPaintEvent *event);
    int lineCount() const                   { return m_lineCount; }
    void setLineCount(int lineCount)        { m_lineCount = lineCount; updateGeometry(); update(); }
private:
    QMargins m_margins;
    int m_lineCount;
};


///////////////////////////////////////////////////////////////////////////////
// SourceWidgetView

// The widget showing the actual text, inside the source widget's viewport.
class SourceWidgetView : public QWidget
{
    Q_OBJECT
public:
    SourceWidgetView(const QMargins &margins, Project &project);

    void setFile(File *file);
    File *file() { return m_file; }

protected:
    void paintEvent(QPaintEvent *event);
    QSize sizeHint() const;
    QSize minimumSizeHint() const { return sizeHint(); }

private:
    QMargins m_margins;
    Project &m_project;
    File *m_file;
    QVector<CXXSyntaxHighlighter::Kind> m_syntaxColoring;
    int m_maxLineLength;
};


///////////////////////////////////////////////////////////////////////////////
// SourceWidget

class SourceWidget : public QScrollArea
{
    Q_OBJECT
public:
    explicit SourceWidget(Project &project, QWidget *parent = 0);
    void setFile(File *file);
    File *file() { return sourceWidgetView().file(); }

private:
    SourceWidgetView &sourceWidgetView();

public:
    void selectIdentifier(int line, int column);

private slots:
    void scrollBarValueChanged();

private:
    void layoutLineArea(void);

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

    SourceWidgetLineArea *m_lineArea;
    Project &m_project;
    friend class SourceWidgetLineArea;
};

} // namespace Nav

#endif // NAV_SOURCEWIDGET_H
