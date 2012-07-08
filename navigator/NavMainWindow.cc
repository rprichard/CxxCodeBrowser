#include "NavMainWindow.h"
#include "ui_NavMainWindow.h"
#include "Program.h"
#include "Source.h"
#include "SourcesJsonReader.h"
#include "NavTableWindow.h"
#include "TableSupplierSourceList.h"
#include <QDebug>
#include <QFile>
#include <QFont>
#include <QString>
#include <QShortcut>
#include <QTimer>
#include <QKeySequence>

NavMainWindow *theMainWindow;

NavMainWindow::NavMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
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

NavMainWindow::~NavMainWindow()
{
    delete ui;
}

void NavMainWindow::showFile(const QString &path)
{
    QFile f(path);
    f.open(QIODevice::ReadOnly);
    ui->sourceWidget->clear();
    ui->sourceWidget->insertPlainText(f.readAll());
    f.close();
}

void NavMainWindow::actionViewSource()
{
    Nav::TableSupplierSourceList *supplier = new Nav::TableSupplierSourceList();
    NavTableWindow *tw = new NavTableWindow(supplier);
    tw->show();
}

void NavMainWindow::actionCommand(const QString &command)
{
    ui->commandWidget->writeLine(QString("You typed [") + command + "]");
}
