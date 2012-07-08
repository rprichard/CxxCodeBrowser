#include "Misc.h"
#include <QDebug>
#include <QMouseEvent>
#include <QPlainTextEdit>

namespace Nav {

void hackDisableDragAndDropByClearingSelection(
        QPlainTextEdit *editor,
        QMouseEvent *event)
{
    // With Qt, if you mouse-down inside selected text (including the blank
    // area to the right of the highlighted text), Qt assumes you want to
    // drag-and-drop the text, so it doesn't change the selection.  This is
    // never what I want -- I always want the selection cleared because I'm
    // either going to edit at the new location (e.g. a mouse click) or make a
    // new selection (e.g. mouse-down-and-move).  Qt has a setDragEnabled
    // method on the QTextControl object used internally by
    // QTextEdit/QPlainTextEdit, but QTextControl is private.  Therefore,
    // clear the selection preemptively so that the default handler can't start
    // a drag-and-drop.
    if (event->button() == Qt::LeftButton &&
            editor->textCursor().hasSelection()) {
        QTextCursor c = editor->textCursor();
        int clickPos = editor->cursorForPosition(event->pos()).position();
        if (c.hasSelection() &&
                clickPos >= c.selectionStart() &&
                clickPos <= c.selectionEnd()) {
            c.clearSelection();
            editor->setTextCursor(c);
        }
    }
}

} // namespace Nav
