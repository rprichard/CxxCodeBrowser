#include "FindBar.h"

#include <QApplication>
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

const int kInfoLabelLeftMarginPx = 3;
const int kInfoLabelRightMarginPx = 3;
const int kInfoLabelVerticalMarginPx = 3;

FindBarEditFrame::FindBarEditFrame(QWidget *parent) : QWidget(parent)
{
    // TODO: Move this...
    QFont f = font();
    f.setPointSize(9);
    setFont(f);

    m_edit = new FindBarEdit(this);
    m_edit->setFrame(false);
    m_edit->setFont(f);
    m_edit->setPlaceholderText(placeholderText);
    QPalette editPalette = m_edit->palette();
    editPalette.setColor(m_edit->backgroundRole(), Qt::transparent);
    m_edit->setPalette(editPalette);

    m_infoLabel = new QLabel(this);
    m_infoLabel->setFont(font());

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFocusProxy(m_edit);

    layoutChildren();
}

QSize FindBarEditFrame::sizeHint() const
{
    QFontMetrics fm = fontMetrics();
    QStyleOptionFrameV2 option;
    initStyleOption(option);
    QSize contentsSize(
                fm.width('x') * 4 + (2 * 2),
                fm.height() + (2 * 1));
    QSize labelSize = m_infoLabel->sizeHint();
    contentsSize.rwidth() += kInfoLabelLeftMarginPx + kInfoLabelRightMarginPx;
    contentsSize.rheight() = std::max(
                contentsSize.height(),
                labelSize.height() + 2 * kInfoLabelVerticalMarginPx);
    contentsSize = contentsSize.expandedTo(QApplication::globalStrut());
    QSize styledSize = style()->sizeFromContents(
                QStyle::CT_LineEdit, &option, contentsSize, m_edit);
    return styledSize;
}

void FindBarEditFrame::initStyleOption(QStyleOptionFrameV2 &option) const
{
    option.initFrom(this);
    option.fontMetrics = fontMetrics();
    option.rect = rect();
    option.lineWidth = style()->pixelMetric(
                QStyle::PM_DefaultFrameWidth, &option, m_edit);
    option.midLineWidth = 0;
    option.features = QStyleOptionFrameV2::None;
}

void FindBarEditFrame::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    QStyleOptionFrameV2 option;
    initStyleOption(option);
    option.state |= QStyle::State_Enabled | QStyle::State_Sunken;
    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter);
}

void FindBarEditFrame::resizeEvent(QResizeEvent *event)
{
    layoutChildren();
}

bool FindBarEditFrame::event(QEvent *event)
{
    if (event->type() == QEvent::LayoutRequest)
        layoutChildren();
    return QWidget::event(event);
}

void FindBarEditFrame::layoutChildren()
{
    QStyleOptionFrameV2 option;
    initStyleOption(option);
    QRect contentRect = style()->subElementRect(
                QStyle::SE_LineEditContents, &option, m_edit);
    QSize infoSize = m_infoLabel->sizeHint();

    int x4 = contentRect.x() + contentRect.width() - kInfoLabelRightMarginPx;
    int x3 = x4 - infoSize.width();
    int x2 = x3 - kInfoLabelLeftMarginPx;
    int x1 = contentRect.x();

    int infoLabelTop = contentRect.y() +
            std::max(0, (contentRect.height() - infoSize.height()) / 2);

    m_edit->setGeometry(x1, contentRect.y(), x2 - x1, contentRect.height());
    m_infoLabel->setGeometry(x3, infoLabelTop, x4 - x3, infoSize.height());
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

    m_editFrame = new FindBarEditFrame;
    layout->addWidget(m_editFrame);
    FindBarEdit *edit = m_editFrame->findBarEdit();

    connect(edit, SIGNAL(returnPressed()), SIGNAL(next()));
    connect(edit, SIGNAL(shiftReturnPressed()), SIGNAL(previous()));
    connect(edit, SIGNAL(escapePressed()), SIGNAL(closeBar()));
    connect(edit, SIGNAL(textChanged(QString)), SLOT(onEditTextChanged()));

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFocusProxy(m_editFrame);

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
    FindBarEdit *edit = m_editFrame->findBarEdit();
    QLabel *infoLabel = m_editFrame->infoLabel();
    if (edit->text().isEmpty()) {
        infoLabel->setText("");
        infoLabel->setVisible(false);
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
        infoLabel->setText(label);
        infoLabel->setVisible(true);
        isError = (count == 0);
    }
    QPalette p;
    if (isError)
        p.setBrush(infoLabel->backgroundRole(), QColor(255, 102, 102));
    else
        p.setBrush(infoLabel->foregroundRole(), QColor(Qt::darkGray));
    infoLabel->setAutoFillBackground(isError);
    infoLabel->setPalette(p);
}

void FindBar::selectAll()
{
    m_editFrame->findBarEdit()->selectAll();
}

void FindBar::onEditTextChanged()
{
    FindBarEdit *edit = m_editFrame->findBarEdit();
    Regex regex(edit->text().toStdString());

    {
        QPalette pal = edit->palette();
        pal.setColor(edit->foregroundRole(),
                     regex.valid() ? palette().color(edit->foregroundRole())
                                   : QColor(Qt::red));
        edit->setPalette(pal);
    }

    if (regex.valid()) {
        m_regex = regex;
        emit regexChanged();
    }
}

} // namespace Nav
