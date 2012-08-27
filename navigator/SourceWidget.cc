#include "SourceWidget.h"
#include <QDebug>
#include <QMenu>
#include <QTextBlock>
#include "Misc.h"
#include "File.h"
#include "Project.h"
#include "ReportRefList.h"
#include "TreeReportWindow.h"

namespace Nav {

SourceWidget::SourceWidget(QWidget *parent) :
    QPlainTextEdit(parent),
    m_file(NULL)
{
    setWordWrapMode(QTextOption::NoWrap);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setTextInteractionFlags(Qt::NoTextInteraction);
    setReadOnly(true);
}

void SourceWidget::setFile(File *file)
{
    if (m_file != file) {
        m_file = file;
        setPlainText(file->content());
    }
}

void SourceWidget::mousePressEvent(QMouseEvent *event)
{
    QTextCursor cursor = cursorForPosition(event->pos());
    cursor = findEnclosingIdentifier(cursor);
    setTextCursor(cursor);
}

void SourceWidget::clearSelection()
{
    QTextCursor cursor = textCursor();
    cursor.clearSelection();
    setTextCursor(cursor);
}

void SourceWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QTextCursor cursor = cursorForPosition(event->pos());
    cursor = findEnclosingIdentifier(cursor);
    if (cursor == textCursor()) {

        // The identifier was clicked..
        // TODO: Go to the definition of the identifier.

    }
    clearSelection();
}

void SourceWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
}

void SourceWidget::mouseMoveEvent(QMouseEvent *event)
{
    QTextCursor cursor = cursorForPosition(event->pos());
    cursor = findEnclosingIdentifier(cursor);
    if (cursor != textCursor())
        clearSelection();
}

void SourceWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QTextCursor cursor = cursorForPosition(event->pos());
    cursor = findEnclosingIdentifier(cursor);
    if (!cursor.selectedText().isEmpty()) {
        QMenu *menu = new QMenu();
        int line = cursor.blockNumber() + 1;
        int column = cursor.selectionStart() - cursor.block().position() + 1;
        QStringList symbols = theProject->querySymbolsAtLocation(m_file, line, column);
        if (symbols.isEmpty()) {
            QAction *action = menu->addAction("No symbols found");
            action->setEnabled(false);
        } else {
            for (QString symbol : symbols) {
                QAction *action = menu->addAction(symbol);
                action->setEnabled(false);
                //action->setSeparator(true);
                if (1) {
                    QFont f = action->font();
                    f.setBold(true);
                    action->setFont(f);
                }
                menu->addSeparator();
                action = menu->addAction("Cross-references...", this, SLOT(actionCrossReferences()));
                action->setData(symbol);
                menu->addSeparator();
            }
        }
        menu->exec(event->globalPos());
        delete menu;
    }
    clearSelection();
}

void SourceWidget::actionCrossReferences()
{
    QAction *action = qobject_cast<QAction*>(sender());
    QString symbol = action->data().toString();
    ReportRefList *r = new ReportRefList(theProject, symbol);
    TreeReportWindow *tw = new TreeReportWindow(r);
    tw->show();
}

static bool isIdentifierChar(QChar ch)
{
    return ch.isLetterOrNumber() || ch == '_';
}

QTextCursor SourceWidget::findEnclosingIdentifier(QTextCursor pt)
{
    QString lineContent = pt.block().text();
    int first = pt.positionInBlock();
    int last = pt.positionInBlock();

    if (first >= lineContent.size())
        return pt;

    if (isIdentifierChar(lineContent[first])) {
        while (first - 1 >= 0 && isIdentifierChar(lineContent[first - 1]))
            first--;
        while (last + 1 < lineContent.size() && isIdentifierChar(lineContent[last + 1]))
            last++;
    }

    pt.setPosition(pt.block().position() + first);
    pt.setPosition(pt.block().position() + last + 1, QTextCursor::KeepAnchor);
    return pt;
}

// Line and column indices are 1-based.
void SourceWidget::selectIdentifier(int line, int column)
{
    // TODO: Do tab stops affect the column?
    QTextCursor c(document());
    c.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line - 1);
    c.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column - 1);
    c = findEnclosingIdentifier(c);
    setTextCursor(c);
}

} // namespace Nav
