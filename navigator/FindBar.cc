#include "FindBar.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionFrameV2>
#include <QToolButton>

#include "Application.h"
#include "Misc.h"

namespace Nav {


///////////////////////////////////////////////////////////////////////////////
// FindBarEdit

const int kInfoLabelLeftMarginPx = 3;
const int kInfoLabelRightMarginPx = 3;

FindBarEdit::FindBarEdit(QWidget *parent) :
    PlaceholderLineEdit(parent),
    m_infoBackground(Qt::transparent)
{
    m_infoForeground = palette().color(foregroundRole());
}

void FindBarEdit::setInfoText(const QString &infoText)
{
    if (infoText == m_infoText)
        return;
    m_infoText = infoText;
    int rightMargin = 0;
    if (!m_infoText.isEmpty()) {
        int textWidth = fontMetrics().width(m_infoText);
        rightMargin =
                kInfoLabelLeftMarginPx +
                kInfoLabelRightMarginPx +
                rightFrameWidth() +
                textWidth;
    }
    setTextMargins(0, 0, rightMargin, 0);
    update();
}

void FindBarEdit::setInfoColors(
        const QColor &infoForeground,
        const QColor &infoBackground)
{
    if (infoForeground == m_infoForeground &&
            infoBackground == m_infoBackground)
        return;
    m_infoForeground = infoForeground;
    m_infoBackground = infoBackground;
    update();
}

int FindBarEdit::rightFrameWidth()
{
    QStyleOptionFrameV2 option;
    initStyleOption(&option);
    QRect subRect = style()->subElementRect(
                QStyle::SE_LineEditContents, &option, this);
    return width() - subRect.x() - subRect.width();
}

void FindBarEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        if (event->modifiers() & Qt::ShiftModifier)
            emit shiftReturnPressed();
        else
            emit returnPressed();
    } else if (event->key() == Qt::Key_Escape) {
        emit escapePressed();
    } else {
        PlaceholderLineEdit::keyPressEvent(event);
    }
}

void FindBarEdit::paintEvent(QPaintEvent *event)
{
    PlaceholderLineEdit::paintEvent(event);
    int textWidth = fontMetrics().width(m_infoText);
    int textX = width() -
            kInfoLabelRightMarginPx -
            rightFrameWidth() -
            textWidth;
    int textHeight = effectiveLineSpacing(fontMetrics());
    int textY = (height() - textHeight) / 2;
    QPainter painter(this);
    painter.save();
    painter.setPen(m_infoForeground);
    painter.fillRect(textX, textY, textWidth, textHeight, m_infoBackground);
    painter.drawText(textX, textY + fontMetrics().ascent(), m_infoText);
    painter.restore();
}


///////////////////////////////////////////////////////////////////////////////
// FindBar

FindBar::FindBar(QWidget *parent) :
    QFrame(parent)
{
    setFont(Application::instance()->defaultFont());
    setFrameShape(QFrame::StyledPanel);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(2);
    layout->setSpacing(2);

    m_edit = new FindBarEdit;
    m_edit->setPlaceholderText(placeholderText);
    layout->addWidget(m_edit);

    connect(m_edit, SIGNAL(returnPressed()), SIGNAL(next()));
    connect(m_edit, SIGNAL(shiftReturnPressed()), SIGNAL(previous()));
    connect(m_edit, SIGNAL(escapePressed()), SIGNAL(closeBar()));
    connect(m_edit, SIGNAL(textChanged(QString)), SLOT(onEditTextChanged()));

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFocusProxy(m_edit);

    QToolButton *b;
    b = makeButton("go-previous", "Previous (Shift-Enter)", "",
                   SIGNAL(previous()));
    layout->addWidget(b);
    b = makeButton("go-next", "Next (Enter)", "",
                   SIGNAL(next()));
    layout->addWidget(b);
    b = makeButton("window-close", "Close", "Close find bar (Esc)",
                   SIGNAL(closeBar()));
    layout->addWidget(b);
}

const Regex &FindBar::regex()
{
    return m_regex;
}

QToolButton *FindBar::makeButton(
        const QString &icon,
        const QString &name,
        const QString &tooltip,
        const char *signal)
{
    QToolButton *button = new QToolButton;
    button->setMinimumSize(QSize(28, 28));
    button->setIcon(QIcon::fromTheme(icon));
    button->setIconSize(QSize(16, 16));
    button->setToolTip(tooltip.isEmpty() ? name : tooltip);
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::NoFocus);
    connect(button, SIGNAL(clicked()), signal);
    return button;
}

void FindBar::setMatchInfo(int index, int count)
{
    bool isError;
    QString infoText;
    if (m_edit->text().isEmpty()) {
        infoText = "";
        isError = false;
    } else {
        if (index == -1) {
            if (count == 1)
                infoText = "1 match";
            else
                infoText = QString("%0 matches").arg(count);
        } else {
            infoText = QString("%0 of %1").arg(index + 1).arg(count);
        }
        isError = (count == 0);
    }
    m_edit->setInfoColors(
                isError ? palette().color(foregroundRole()) : Qt::darkGray,
                isError ? QColor(255, 102, 102) : Qt::transparent);
    m_edit->setInfoText(infoText);
}

void FindBar::selectAll()
{
    m_edit->selectAll();
}

void FindBar::onEditTextChanged()
{
    Regex regex(m_edit->text().toStdString());

    {
        QPalette pal = m_edit->palette();
        pal.setColor(m_edit->foregroundRole(),
                     regex.valid() ? palette().color(m_edit->foregroundRole())
                                   : QColor(Qt::red));
        m_edit->setPalette(pal);
    }

    if (regex.valid()) {
        m_regex = regex;
        emit regexChanged();
    }
}

} // namespace Nav
