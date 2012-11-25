#include "PlaceholderLineEdit.h"
#include "Misc.h"

#include <QPainter>

namespace Nav {

PlaceholderLineEdit::PlaceholderLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
}

void PlaceholderLineEdit::setPlaceholderText(const QString &placeholderText)
{
    m_placeholderText = placeholderText;
    update();
}

QString PlaceholderLineEdit::placeholderText() const
{
    return m_placeholderText;
}

void PlaceholderLineEdit::paintEvent(QPaintEvent *event)
{
    QLineEdit::paintEvent(event);
    if (text().isEmpty()) {
        QPainter p(this);
        QFont f = font();
        f.setItalic(true);
        p.setFont(f);
        p.setPen(QColor(Qt::lightGray));
        p.drawText(6,
                   fontMetrics().ascent() +
                        (height() - effectiveLineSpacing(fontMetrics())) / 2,
                   m_placeholderText);
    }
}

} // namespace Nav
