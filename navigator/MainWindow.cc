#include "MainWindow.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFont>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QShortcut>
#include <QSplitter>
#include <QString>
#include <QTreeView>

#include "File.h"
#include "FileManager.h"
#include "FolderWidget.h"
#include "GotoWindow.h"
#include "Project.h"
#include "ReportFileList.h"
#include "ReportRefList.h"
#include "SourceWidget.h"
#include "TreeReportWindow.h"
#include "ui_MainWindow.h"

namespace Nav {

const int kDefaultSideBarSizePx = 300;

MainWindow *theMainWindow;

MainWindow::MainWindow(Project &project, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_sourceWidget = new SourceWidget(project);

    m_folderWidget = new FolderWidget(project.fileManager());
    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->addWidget(m_folderWidget);
    m_splitter->addWidget(m_sourceWidget);
    setCentralWidget(m_splitter);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    QList<int> sizes;
    sizes << kDefaultSideBarSizePx;
    sizes << 1;
    m_splitter->setSizes(sizes);

    connect(ui->action_File_List, SIGNAL(triggered()), this, SLOT(actionViewFileList()));
    connect(m_sourceWidget,
            SIGNAL(fileChanged(File*)),
            SLOT(sourceWidgetFileChanged(File*)));
    connect(m_sourceWidget,
            SIGNAL(areBackAndForwardEnabled(bool&,bool&)),
            SLOT(areBackAndForwardEnabled(bool&,bool&)));
    connect(m_folderWidget, SIGNAL(selectionChanged()), SLOT(folderWidgetSelectionChanged()));
    connect(m_sourceWidget, SIGNAL(goBack()), SLOT(actionBack()));
    connect(m_sourceWidget, SIGNAL(goForward()), SLOT(actionForward()));
    connect(m_sourceWidget, SIGNAL(copyFilePath()), SLOT(actionCopyFilePath()));
    connect(m_sourceWidget, SIGNAL(revealInSideBar()), SLOT(actionRevealInSideBar()));

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
    if (ref.isNull())
        return;
    History::Location loc = currentLocation();
    m_sourceWidget->setFile(&ref.file());
    m_sourceWidget->selectIdentifier(ref.line(), ref.column());
    m_history.recordJump(loc, currentLocation());
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
    m_folderWidget->selectFile(file);
}

void MainWindow::folderWidgetSelectionChanged()
{
    File *file = m_folderWidget->selectedFile();
    navigateToFile(file);
}

void MainWindow::areBackAndForwardEnabled(
        bool &backEnabled, bool &forwardEnabled)
{
    backEnabled = m_history.canGoBack();
    forwardEnabled = m_history.canGoForward();
}

void MainWindow::actionCopyFilePath()
{
    File *file = m_sourceWidget->file();
    if (file != NULL)
        QApplication::clipboard()->setText(file->path());
}

void MainWindow::actionRevealInSideBar()
{
    File *file = m_sourceWidget->file();
    if (file != NULL) {
        if (m_splitter->sizes()[0] == 0) {
            QList<int> sizes;
            sizes << kDefaultSideBarSizePx;
            sizes << 1;
            m_splitter->setSizes(sizes);
        }
        m_folderWidget->ensureItemVisible(file);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QApplication::quit();
}

} // namespace Nav
