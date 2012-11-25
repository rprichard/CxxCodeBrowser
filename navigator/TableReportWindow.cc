#include "TableReportWindow.h"

#include <QApplication>
#include <QKeyEvent>
#include <QVBoxLayout>

#include "Misc.h"
#include "PlaceholderLineEdit.h"
#include "Regex.h"
#include "TableReport.h"
#include "TableReportView.h"

namespace Nav {

TableReportWindow::TableReportWindow(QWidget *parent) :
    QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);

    new QVBoxLayout(this);
    m_view = new TableReportView;
    m_filterBox = new PlaceholderLineEdit;
    m_filterBox->setPlaceholderText(placeholderText);

    // TODO: centralize font settings.
    QFont font = m_filterBox->font();
    font.setPointSize(9);
    m_filterBox->setFont(font);

    setFilterBoxVisible(false);
    layout()->setMargin(2);
    layout()->addWidget(m_filterBox);
    layout()->addWidget(m_view);
    connect(m_filterBox, SIGNAL(textChanged(QString)), this, SLOT(filterTextChanged()));

    m_filterBox->installEventFilter(this);
}

// The TableReportWindow does not take ownership of the TableReport.
void TableReportWindow::setTableReport(TableReport *report)
{
    m_view->setTableReport(report);
    setWindowTitle(report != NULL ? report->title() : "");
}

void TableReportWindow::filterTextChanged()
{
    Regex regex(m_filterBox->text().toStdString().c_str());

    {
        QPalette pal;
        if (!regex.valid())
            pal.setColor(m_filterBox->foregroundRole(), QColor(Qt::red));
        m_filterBox->setPalette(pal);
    }

    if (regex.valid())
        m_view->setFilter(regex);
}

void TableReportWindow::setFilterBoxVisible(bool visible)
{
    m_filterBox->setVisible(visible);
    m_view->setFocusPolicy(visible ? Qt::NoFocus : Qt::WheelFocus);
}

// Intercept some navigation keyboard events directed at the filter box and
// send them to the table view instead.
bool TableReportWindow::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress ||
            event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        const int key = keyEvent->key();
        const bool ctrl = keyEvent->modifiers() & Qt::ControlModifier;
        const bool alt = keyEvent->modifiers() & Qt::AltModifier;
        if (key == Qt::Key_Enter ||
                key == Qt::Key_Return ||
                key == Qt::Key_Up ||
                key == Qt::Key_Down ||
                key == Qt::Key_PageUp ||
                key == Qt::Key_PageDown ||
                (ctrl && key == Qt::Key_Home) ||
                (ctrl && key == Qt::Key_End) ||
                (alt && key == Qt::Key_Left) ||
                (alt && key == Qt::Key_Right)) {
            QApplication::sendEvent(m_view, event);
            return true;
        }
    }
    return false;
}

void TableReportWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        close();
}

} // namespace Nav
