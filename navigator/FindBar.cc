#include "FindBar.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionFrameV2>
#include <QToolButton>

#include "Misc.h"

namespace Nav {


///////////////////////////////////////////////////////////////////////////////
// FindBarEdit

FindBarEdit::FindBarEdit(QWidget *parent) : PlaceholderLineEdit(parent)
{
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


///////////////////////////////////////////////////////////////////////////////
// FindBarEditFrame

FindBarEditFrame::FindBarEditFrame(QWidget *parent) : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 0, 4, 0);
    layout->setSpacing(2);
}

void FindBarEditFrame::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QStyleOptionFrameV2 option;
    initStyleOption(option);
    if (parentWidget() && parentWidget()->hasFocus())
        option.state |= QStyle::State_HasFocus;
    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter);
}

int FindBarEditFrame::frameWidth()
{
    QStyleOptionFrameV2 option;
    initStyleOption(option);
    return option.lineWidth;
}

void FindBarEditFrame::initStyleOption(QStyleOptionFrameV2 &option)
{
    option.initFrom(this);
    option.rect = contentsRect();
    option.lineWidth = style()->pixelMetric(
                QStyle::PM_DefaultFrameWidth, &option, this);
    option.midLineWidth = 0;
}


///////////////////////////////////////////////////////////////////////////////
// FindBar

FindBar::FindBar(QWidget *parent) :
    QFrame(parent)
{
    // TODO: Move this...
    QFont f = font();
    f.setPointSize(9);
    setFont(f);

    setFrameShape(QFrame::StyledPanel);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(2);
    layout->setSpacing(2);

    FindBarEditFrame *editFrame = new FindBarEditFrame;
    layout->addWidget(editFrame);
    m_edit = new FindBarEdit(editFrame);
    m_edit->setFrame(false);
    m_edit->setFont(font());
    m_edit->setPlaceholderText(placeholderText);
    connect(m_edit, SIGNAL(returnPressed()), SIGNAL(next()));
    connect(m_edit, SIGNAL(shiftReturnPressed()), SIGNAL(previous()));
    connect(m_edit, SIGNAL(escapePressed()), SIGNAL(closeBar()));
    connect(m_edit, SIGNAL(textChanged(QString)), SLOT(onEditTextChanged()));
    editFrame->layout()->addWidget(m_edit);
    editFrame->layout()->addWidget(m_infoLabel = new QLabel);
    m_infoLabel->setFont(font());
    editFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_infoLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
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
    if (m_edit->text().isEmpty()) {
        m_infoLabel->setText("");
        m_infoLabel->setVisible(false);
        isError = false;
    } else {
        QString label;
        if (index == -1) {
            if (count == 1)
                label = "1 match";
            else
                label = QString("%0 matches").arg(count);
        } else {
            label = QString("%0 of %1").arg(index + 1).arg(count);
        }
        m_infoLabel->setText(label);
        m_infoLabel->setVisible(true);
        isError = (count == 0);
    }
    QPalette p;
    if (isError)
        p.setBrush(m_infoLabel->backgroundRole(), QColor(255, 102, 102));
    else
        p.setBrush(m_infoLabel->foregroundRole(), QColor(Qt::darkGray));
    m_infoLabel->setAutoFillBackground(isError);
    m_infoLabel->setPalette(p);
}

void FindBar::selectAll()
{
    m_edit->selectAll();
}

void FindBar::onEditTextChanged()
{
    Regex regex(m_edit->text().toStdString());

    {
        QPalette pal;
        if (!regex.valid())
            pal.setColor(m_edit->foregroundRole(), QColor(Qt::red));
        m_edit->setPalette(pal);
    }

    if (regex.valid()) {
        m_regex = regex;
        emit regexChanged();
    }
}

} // namespace Nav
