#include "SourceWidget.h"
#include "Misc.h"
#include <QDebug>

namespace Nav {

SourceWidget::SourceWidget(QWidget *parent) :
    QPlainTextEdit(parent)
{
}

void SourceWidget::mousePressEvent(QMouseEvent *event)
{
    Nav::hackDisableDragAndDropByClearingSelection(this, event);
    QPlainTextEdit::mousePressEvent(event);
}

} // namespace Nav
