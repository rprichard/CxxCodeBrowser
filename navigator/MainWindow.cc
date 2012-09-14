#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Project.h"
#include "File.h"
#include "FileManager.h"
#include "TreeReportWindow.h"
#include "ReportFileList.h"
#include "ReportRefList.h"
#include "SourceWidget.h"
#include <QDebug>
#include <QFile>
#include <QFont>
#include <QString>
#include <QShortcut>
#include <QTimer>
#include <QKeySequence>

#include "GotoWindow.h"

namespace Nav {

MainWindow *theMainWindow;

MainWindow::MainWindow(Project &project, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_sourceWidget = new SourceWidget(project);
    ui->verticalLayout->addWidget(m_sourceWidget);

    // Configure the widgets to use a small monospace font.  If we
    // don't force integer metrics, then the font may have a fractional
    // width, which prevents us from configuring appropriate tab stops.
    QFont font;
    font.setFamily("Monospace");
    font.setPointSize(8);
    font.setStyleStrategy(QFont::ForceIntegerMetrics);
    //ui->sourceWidget->setFont(font);
    //ui->commandWidget->setFont(font);
    //QFontMetrics fontMetrics(ui->sourceWidget->font());
    //int fontWidth = fontMetrics.width(' ');
    //ui->sourceWidget->setTabStopWidth(fontWidth * 8);

    /*
    // Make the command pane as small as allowed.
    QList<int> newSizes;
    newSizes << height();
    newSizes << 1;
    ui->splitter->setSizes(newSizes);
    */

    connect(ui->action_File_List, SIGNAL(triggered()), this, SLOT(actionViewFileList()));
    //connect(ui->commandWidget, SIGNAL(commandEntered(QString)), SLOT(actionCommand(QString)));

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

void MainWindow::actionCommand(const QString &commandIn)
{
    QString command = commandIn.trimmed();

    if (command == "files") {
        actionViewFileList();
    } else if (command.startsWith("xref ")) {
        QString symbolName = command.mid(strlen("xref "));

        // TODO: We need to print this message eventually.
        //ui->commandWidget->writeLine(QString("Symbol not found: ") + symbolName);

        ReportRefList *r = new ReportRefList(theProject, symbolName);
        TreeReportWindow *tw = new TreeReportWindow(r);
        tw->show();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QApplication::quit();
}

} // namespace Nav
