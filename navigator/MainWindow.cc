#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Project.h"
#include "Symbol.h"
#include "SymbolTable.h"
#include "File.h"
#include "FileManager.h"
#include "CSource.h"
#include "SourcesJsonReader.h"
#include "TreeReportWindow.h"
#include "ReportCSources.h"
#include "ReportRefList.h"
#include <QDebug>
#include <QFile>
#include <QFont>
#include <QString>
#include <QShortcut>
#include <QTimer>
#include <QKeySequence>

namespace Nav {

MainWindow *theMainWindow;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    file(NULL)
{
    ui->setupUi(this);

    // Configure the widgets to use a small monospace font.  If we
    // don't force integer metrics, then the font may have a fractional
    // width, which prevents us from configuring appropriate tab stops.
    QFont font;
    font.setFamily("Monospace");
    font.setPointSize(8);
    font.setStyleStrategy(QFont::ForceIntegerMetrics);
    ui->sourceWidget->setFont(font);
    ui->commandWidget->setFont(font);
    QFontMetrics fontMetrics(ui->sourceWidget->font());
    int fontWidth = fontMetrics.width(' ');
    ui->sourceWidget->setTabStopWidth(fontWidth * 8);

    // Make the command pane as small as allowed.
    QList<int> newSizes;
    newSizes << height();
    newSizes << 1;
    ui->splitter->setSizes(newSizes);

    connect(ui->action_Source_List, SIGNAL(triggered()), this, SLOT(actionViewSource()));
    connect(ui->commandWidget, SIGNAL(commandEntered(QString)), SLOT(actionCommand(QString)));

    // Register Ctrl+Q.
    QShortcut *shortcut = new QShortcut(QKeySequence("Ctrl+Q"), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(close()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showFile(const QString &path)
{
    Nav::File *newFile = Nav::theProject->fileManager->file(path);
    if (newFile != NULL && newFile != file) {
        file = newFile;
        ui->sourceWidget->setPlainText(newFile->content);
    }
}

void MainWindow::selectText(int line, int column, int size)
{
    // TODO: Do tab stops affect the column?
    QTextCursor c(ui->sourceWidget->document());
    c.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line - 1);
    c.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column - 1);
    c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, size);
    ui->sourceWidget->setTextCursor(c);
}

void MainWindow::actionViewSource()
{
    ReportCSources *r = new ReportCSources(theProject);
    TreeReportWindow *tw = new TreeReportWindow(r);
    tw->show();
}

void MainWindow::actionCommand(const QString &commandIn)
{
    QString command = commandIn.trimmed();

    if (command == "sources") {
        actionViewSource();
    } else if (command.startsWith("xref ")) {
        QString symbolName = command.mid(strlen("xref "));
        Nav::Symbol *symbol = Nav::theProject->symbolTable->symbol(symbolName);
        if (symbol == NULL) {
            ui->commandWidget->writeLine(QString("Symbol not found: ") + symbolName);
        } else {
            ReportRefList *r = new ReportRefList(symbol);
            TreeReportWindow *tw = new TreeReportWindow(r);
            tw->show();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QApplication::quit();
}

} // namespace Nav
