#include "CommandWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QMenu>

#include "Misc.h"

namespace Nav {

CommandWidget::CommandWidget(QWidget *parent) :
    QPlainTextEdit(parent),
    prompt("> ")
{
    appendPlainText(prompt);
}

void CommandWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() & Qt::AltModifier) {
        event->ignore();
        return;
    }

    QTextCursor cursorLastLine(document());
    cursorLastLine.movePosition(QTextCursor::End);
    cursorLastLine.movePosition(QTextCursor::StartOfLine);

    QTextCursor initialCursor = textCursor();

    Qt::KeyboardModifiers modifiers = event->modifiers() &
            (Qt::ControlModifier | Qt::ShiftModifier);
    const bool control = modifiers & Qt::ControlModifier;
    const bool shift = modifiers & Qt::ShiftModifier;

    // Handle ESC.
    if (event->key() == Qt::Key_Escape) {
        QTextCursor c(document());
        c.setPosition(cmdPosition());
        c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        c.removeSelectedText();
        setTextCursor(c);
        ensureCursorVisible();
        return;
    }

    // Handle backspace/delete.
    if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) {
        QTextCursor c = textCursor();
        if (initialCursor.hasSelection()) {
            removeText(c.selectionStart(), c.selectionEnd());
            return;
        } else {
            QTextCursor stop = c;
            if (event->key() == Qt::Key_Backspace) {
                stop.movePosition(
                            control ? QTextCursor::PreviousWord :
                                      QTextCursor::PreviousCharacter);
            } else if (event->key() == Qt::Key_Delete) {
                stop.movePosition(QTextCursor::NextCharacter);
            }
            removeText(c.position(), stop.position());
            ensureCursorVisible();
            return;
        }
    }

    // Handle Left/Right.
    {
        QTextCursor::MoveMode mode = shift ? QTextCursor::KeepAnchor :
                                             QTextCursor::MoveAnchor;
        if (event->key() == Qt::Key_Left) {
            moveCmdCursor(control ? QTextCursor::PreviousWord : QTextCursor::Left, mode);
            return;
        } else if (event->key() == Qt::Key_Right) {
            moveCmdCursor(control ? QTextCursor::NextWord : QTextCursor::Right, mode);
            return;
        }
    }

    // Handle Ctrl keys.
    if (control) {
        if (event->key() == Qt::Key_A) {
            moveCmdCursor(QTextCursor::Start);
            return;
        } else  if (event->key() == Qt::Key_E) {
            moveCmdCursor(QTextCursor::End);
            return;
        } else if (event->key() == Qt::Key_C) {
            copy();
            return;
        } else if (event->key() == Qt::Key_V) {
            actionPaste();
            return;
        }
    }

    // Handle character insertion.
    if (!control && event->text().length() == 1) {
        QChar ch = event->text()[0];
        if (ch == '\r')
            ch = '\n';
        typeChar(ch);
        return;
    }

    event->ignore();
}

void CommandWidget::mousePressEvent(QMouseEvent *event)
{
    Nav::hackDisableDragAndDropByClearingSelection(this, event);

    if (event->button() == Qt::LeftButton ||
            event->button() == Qt::RightButton) {
        QPlainTextEdit::mousePressEvent(event);
    } else if (event->button() == Qt::MiddleButton) {
        foreach (QChar ch, QApplication::clipboard()->text(QClipboard::Selection))
            typeChar(ch);
    } else {
        event->ignore();
    }
}

void CommandWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton ||
            event->button() == Qt::RightButton) {
        QPlainTextEdit::mousePressEvent(event);
    } else {
        event->ignore();
    }
}

void CommandWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = new QMenu();
    menu->addAction("&Copy", this, SLOT(copy()));
    menu->addAction("&Paste", this, SLOT(actionPaste()));
    menu->addAction("Select All", this, SLOT(selectAll()));
    menu->exec(event->globalPos());
    delete menu;
}

void CommandWidget::actionPaste()
{
    foreach (QChar ch, QApplication::clipboard()->text())
        typeChar(ch);
}

void CommandWidget::typeChar(QChar ch)
{
    // Filter out invalid characters.  0x7f is the DEL character.
    if (!(ch == '\t' || ch == '\n' || ch >= ' ') || ch == '\x7f') {
        return;
    }

    bool hasCommand = false;
    QString command;

    if (ch == '\n') {
        QTextCursor c1(document());
        c1.setPosition(cmdPosition());
        c1.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        hasCommand = true;
        command = c1.selectedText();
        c1.movePosition(QTextCursor::End);
        c1.insertText("\n");
        c1.insertText(prompt);
        setTextCursor(c1);
    } else {
        QTextCursor c = textCursor();
        if (c.hasSelection()) {
            removeText(c.selectionStart(), c.selectionEnd());
        }
        c = textCursor();
        if (c.position() < cmdPosition())
            c.movePosition(QTextCursor::End);
        c.clearSelection();
        setTextCursor(c);
        c.insertText(QString(ch));
    }

    ensureCursorVisible();

    // Delay command emission until we've shown the prompt for the next line.
    if (hasCommand)
        emit commandEntered(command);
}

void CommandWidget::moveCmdCursor(
        QTextCursor::MoveOperation op,
        QTextCursor::MoveMode mode)
{
    QTextCursor c = textCursor();
    c.movePosition(op, mode);
    if (c.position() < cmdPosition())
        c.setPosition(cmdPosition(), mode);
    setTextCursor(c);
    ensureCursorVisible();
}

int CommandWidget::cmdPosition()
{
    QTextCursor c(document());
    c.movePosition(QTextCursor::End);
    c.movePosition(QTextCursor::StartOfLine);
    c.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, prompt.size());
    return c.position();
}

int CommandWidget::endPosition()
{
    QTextCursor c(document());
    c.movePosition(QTextCursor::End);
    return c.position();
}

bool CommandWidget::removeText(int startPos, int stopPos)
{
    int p1 = cmdPosition();
    int p2 = endPosition();
    startPos = std::min(p2, std::max(p1, startPos));
    stopPos = std::min(p2, std::max(p1, stopPos));
    if (startPos == stopPos)
        return false;
    QTextCursor c(document());
    c.setPosition(startPos);
    c.setPosition(stopPos, QTextCursor::KeepAnchor);
    c.removeSelectedText();
    setTextCursor(c);
    return true;
}

void CommandWidget::writeLine(const QString &text)
{
    int initialPos = textCursor().position();
    QTextCursor c(document());
    c.movePosition(QTextCursor::End);
    c.movePosition(QTextCursor::StartOfLine);
    c.insertText(text);
    c.insertText("\n");
    int newPos = textCursor().position();
    if (newPos != initialPos)
        ensureCursorVisible();
}

} // namespace Nav
