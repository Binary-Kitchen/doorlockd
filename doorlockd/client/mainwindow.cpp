#include "config.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _greenLED(false);
    _redLED(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setClientmessage(const Clientmessage &msg)
{
    ui->qrwidget->setQRData(msg.token());
    ui->tokenLabel->setText(QString::fromStdString(msg.token()));

    if (msg.isOpen()) {
        _greenLED(true);
        _redLED(false);
    } else {
        _greenLED(false);
        _redLED(true);
    }
}

void MainWindow::_greenLED(const bool on)
{
    if (on)
        ui->greenLED->setPixmap(QPixmap(IMAGE_LOCATION "led-green-on.png"));
    else
        ui->greenLED->setPixmap(QPixmap(IMAGE_LOCATION "led-green-off.png"));
}

void MainWindow::_redLED(const bool on)
{
    if (on)
        ui->redLED->setPixmap(QPixmap(IMAGE_LOCATION "led-red-on.png"));
    else
        ui->redLED->setPixmap(QPixmap(IMAGE_LOCATION "led-red-off.png"));
}
