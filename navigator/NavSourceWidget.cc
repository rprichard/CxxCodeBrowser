#include "NavSourceWidget.h"
#include "Misc.h"
#include <QDebug>

NavSourceWidget::NavSourceWidget(QWidget *parent) :
    QPlainTextEdit(parent)
{
}

void NavSourceWidget::mousePressEvent(QMouseEvent *event)
{
    Nav::hackDisableDragAndDropByClearingSelection(this, event);
    QPlainTextEdit::mousePressEvent(event);
}
