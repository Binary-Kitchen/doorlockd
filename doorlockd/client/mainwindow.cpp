#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setClientmessage(const Clientmessage &msg)
{
    ui->qrwidget->setQRData(msg.token());
    ui->tokenLabel->setText(QString::fromStdString(msg.token()));
}
