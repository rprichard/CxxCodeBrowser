#include "Misc.h"

#include <QFontMetrics>
#include <QIcon>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QString>
#include <algorithm>

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

// Sometimes the line spacing is smaller than the height, which makes the text
// cramped.  When this happens, use the height instead.  (I think Qt already
// does this -- look for QTextLayout and a negative leading()).
int effectiveLineSpacing(const QFontMetrics &fm)
{
    return std::max(fm.height(), fm.lineSpacing());
}

// On my Linux Mint 13 system, QIcon::themeName identifies my theme as
// "hicolor".  The preferences pane, on the other hand, says my icon theme is
// "MATE".  In any case, the /usr/share/icons/hicolor directory lacks most of
// the icons I care about, so if I don't see the icons I want, I'm going to try
// switching the icon theme to gnome.
void hackSwitchIconThemeToTheOneWithIcons()
{
    QString originalThemeName = QIcon::themeName();
    QIcon previous = QIcon::fromTheme("go-previous");
    QIcon next = QIcon::fromTheme("go-next");
    if (previous.isNull() && next.isNull()) {
        QIcon::setThemeName("gnome");
        previous = QIcon::fromTheme("go-previous");
        next = QIcon::fromTheme("go-next");
        if (previous.isNull() || next.isNull()) {
            // It didn't work, so switch back.
            QIcon::setThemeName(originalThemeName);
        }
    }
}

const char placeholderText[] =
        "Regex filter (RE2). "
        "Case-sensitive if a capital letter exists.";

} // namespace Nav
