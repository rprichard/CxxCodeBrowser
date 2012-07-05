#include "NavMainWindow.h"
#include "ui_NavMainWindow.h"
#include "Program.h"
#include "Source.h"
#include "SourcesJsonReader.h"
#include <QFile>
#include <QFont>

NavMainWindow *theMainWindow;

NavMainWindow::NavMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Configure the textEdit widget to use a small monospace font.  If we
    // don't force integer metrics, then the font may have a fractional
    // width, which prevents us from configuring appropriate tab stops.
    QFont font;
    font.setFamily("Monospace");
    font.setPointSize(8);
    font.setStyleStrategy(QFont::ForceIntegerMetrics);
    ui->textEdit->setFont(font);
    QFontMetrics fontMetrics(ui->textEdit->font());
    int fontWidth = fontMetrics.width(' ');
    ui->textEdit->setTabStopWidth(fontWidth * 8);
}

NavMainWindow::~NavMainWindow()
{
    delete ui;
}

void NavMainWindow::showFile(const std::string &path)
{
    QFile f(QString::fromStdString(path));
    f.open(QIODevice::ReadOnly);
    ui->textEdit->setText(f.readAll());
    f.close();
}
