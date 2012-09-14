#include "MainWindow.h"

#include <QFile>
#include <QFont>
#include <QKeySequence>
#include <QShortcut>
#include <QString>

#include "File.h"
#include "FileManager.h"
#include "GotoWindow.h"
#include "Project.h"
#include "ReportFileList.h"
#include "ReportRefList.h"
#include "SourceWidget.h"
#include "TreeReportWindow.h"
#include "ui_MainWindow.h"

namespace Nav {

MainWindow *theMainWindow;

MainWindow::MainWindow(Project &project, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_sourceWidget = new SourceWidget(project);
    ui->verticalLayout->addWidget(m_sourceWidget);

    connect(ui->action_File_List, SIGNAL(triggered()), this, SLOT(actionViewFileList()));
    connect(m_sourceWidget,
            SIGNAL(fileChanged(File*)),
            SLOT(sourceWidgetFileChanged(File*)));
    connect(m_sourceWidget,
            SIGNAL(areBackAndForwardEnabled(bool&,bool&)),
            SLOT(areBackAndForwardEnabled(bool&,bool&)));
    connect(m_sourceWidget, SIGNAL(goBack()), SLOT(actionBack()));
    connect(m_sourceWidget, SIGNAL(goForward()), SLOT(actionForward()));

    // Keyboard shortcuts.
    QShortcut *shortcut;
    shortcut = new QShortcut(QKeySequence("Ctrl+Q"), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(close()));
    shortcut = new QShortcut(QKeySequence("Ctrl+S"), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(actionOpenGotoWindow()));
    shortcut = new QShortcut(QKeySequence("Alt+Left"), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(actionBack()));
    shortcut = new QShortcut(QKeySequence("Alt+Right"), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(actionForward()));

    ui->toolBar->addAction(QIcon::fromTheme("go-previous"), "Back", this, SLOT(actionBack()));
    ui->toolBar->addAction(QIcon::fromTheme("go-next"), "Forward", this, SLOT(actionForward()));
}

MainWindow::~MainWindow()
{
    qDeleteAll(m_textWidthCalculatorCache);
    delete ui;
}

History::Location MainWindow::currentLocation()
{
    return History::Location(
                m_sourceWidget->file(),
                m_sourceWidget->viewportOrigin());
}

void MainWindow::navigateToFile(File *file)
{
    if (file == NULL)
        return;
    History::Location loc = currentLocation();
    m_sourceWidget->setFile(file);
    m_history.recordJump(loc, currentLocation());
}

void MainWindow::navigateToRef(const Ref &ref)
{
    if (ref.file == NULL)
        return;
    History::Location loc = currentLocation();
    m_sourceWidget->setFile(ref.file);
    m_sourceWidget->selectIdentifier(ref.line, ref.column);
    m_history.recordJump(loc, currentLocation());
}

TextWidthCalculator &MainWindow::getCachedTextWidthCalculator(const QFont &font)
{
    if (!m_textWidthCalculatorCache.contains(font)) {
        m_textWidthCalculatorCache[font] =
                new TextWidthCalculator(QFontMetricsF(font));
    }
    return *m_textWidthCalculatorCache[font];
}

void MainWindow::actionViewFileList()
{
    ReportFileList *r = new ReportFileList(theProject);
    TreeReportWindow *tw = new TreeReportWindow(r);
    tw->show();
}

void MainWindow::actionOpenGotoWindow()
{
    GotoWindow *gw = new GotoWindow(*Nav::theProject);
    gw->show();
}

void MainWindow::actionBack()
{
    if (m_history.canGoBack()) {
        m_history.recordLocation(currentLocation());
        const History::Location *loc = m_history.goBack();
        m_sourceWidget->setFile(loc->file);
        m_sourceWidget->setViewportOrigin(loc->offset);
    }
}

void MainWindow::actionForward()
{
    if (m_history.canGoForward()) {
        m_history.recordLocation(currentLocation());
        const History::Location *loc = m_history.goForward();
        m_sourceWidget->setFile(loc->file);
        m_sourceWidget->setViewportOrigin(loc->offset);
    }
}

void MainWindow::sourceWidgetFileChanged(File *file)
{
    setWindowTitle(file->path() + " - Navigator");
}

void MainWindow::areBackAndForwardEnabled(
        bool &backEnabled, bool &forwardEnabled)
{
    backEnabled = m_history.canGoBack();
    forwardEnabled = m_history.canGoForward();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QApplication::quit();
}

} // namespace Nav
