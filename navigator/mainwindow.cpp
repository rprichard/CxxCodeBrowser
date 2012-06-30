#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QFile f("/home/rprichard/llvm/tools/clang/lib/AST/APValue.cpp");
    f.open(QIODevice::ReadOnly);
    QString qs(f.readAll());
    ui->textEdit->setText("abc");
    ui->textEdit->setText(qs);
}

MainWindow::~MainWindow()
{
    delete ui;
}
