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

    // Register Ctrl+Q.
    QShortcut *shortcut;
    shortcut = new QShortcut(QKeySequence("Ctrl+Q"), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(close()));

    shortcut = new QShortcut(QKeySequence("Ctrl+S"), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(actionOpenGotoWindow()));
}

MainWindow::~MainWindow()
{
    qDeleteAll(m_textWidthCalculatorCache);
    delete ui;
}

void MainWindow::navigateToFile(File *file)
{
    if (file == NULL)
        return;
    m_sourceWidget->setFile(file);
}

void MainWindow::navigateToRef(const Ref &ref)
{
    if (ref.file == NULL)
        return;
    m_sourceWidget->setFile(ref.file);
    m_sourceWidget->selectIdentifier(ref.line, ref.column);
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    QApplication::quit();
}

} // namespace Nav
