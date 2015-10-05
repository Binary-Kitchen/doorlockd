#include "config.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _LED(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setClientmessage(const Clientmessage &msg)
{
    ui->qrwidget->setQRData(msg.token());
    ui->tokenLabel->setText(QString::fromStdString(msg.token()));

    _LED(msg.isOpen());
}

void MainWindow::_LED(const bool on)
{
    if (on)
        ui->LED->setPixmap(QPixmap(IMAGE_LOCATION "led-green.png"));
    else
        ui->LED->setPixmap(QPixmap(IMAGE_LOCATION "led-red.png"));
}
