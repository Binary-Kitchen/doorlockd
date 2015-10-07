#include "config.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

#define PLAY(file) \
    system("play -q " file " &")

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

    const auto &doormsg = msg.doormessage();

    _LED(msg.isOpen());

    if (_oldMessage.isOpen() && !msg.isOpen()) {
        // regular close
        PLAY(SOUND_LOCK);
    } else if (!_oldMessage.isOpen() && msg.isOpen()) {
        // regular open
        PLAY(SOUND_UNLOCK);
    } else {
        // no change
    }

    if (doormsg.isEmergencyUnlock) {
        PLAY(SOUND_EMERGENCY_UNLOCK);
    } else if (doormsg.isLockButton) {
        PLAY(SOUND_LOCK_BUTTON);
    } else if (doormsg.isUnlockButton) {
        if (msg.isOpen()) {
            PLAY(SOUND_ZONK);
        } else {
            PLAY(SOUND_UNLOCK_BUTTON);
        }
    }

    _oldMessage = msg;
}

void MainWindow::_LED(const bool on)
{
    if (on)
        ui->LED->setPixmap(QPixmap(IMAGE_LOCATION "led-green.png"));
    else
        ui->LED->setPixmap(QPixmap(IMAGE_LOCATION "led-red.png"));
}
