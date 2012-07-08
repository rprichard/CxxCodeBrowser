#ifndef NAVCOMMANDWIDGET_H
#define NAVCOMMANDWIDGET_H

#include <QPlainTextEdit>

class NavCommandWidget : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit NavCommandWidget(QWidget *parent = 0);

    void keyPressEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);

private slots:
    void actionCopy();

signals:
    void commandEntered(const QString &command);

public:
    void typeChar(QChar ch);
    void writeLine(const QString &text);
    void moveCmdCursor(
            QTextCursor::MoveOperation op,
            QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);

private:
    int cmdPosition();
    int endPosition();
    bool removeText(int startPos, int stopPos);

private:
    QString prompt;
};

#endif // NAVCOMMANDWIDGET_H
