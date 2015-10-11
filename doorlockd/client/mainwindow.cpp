#include "config.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindow),
    _soundLock(Wave::fromFile(SOUND_LOCK)),
    _soundUnlock(Wave::fromFile(SOUND_UNLOCK)),
    _soundEmergencyUnlock(Wave::fromFile(SOUND_EMERGENCY_UNLOCK)),
    _soundZonk(Wave::fromFile(SOUND_ZONK)),
    _soundLockButton(Wave::fromFile(SOUND_LOCK_BUTTON)),
    _soundUnlockButton(Wave::fromFile(SOUND_UNLOCK_BUTTON))
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
        _soundLock.playAsync();
    } else if (!_oldMessage.isOpen() && msg.isOpen()) {
        // regular open
        _soundUnlock.playAsync();
    } else {
        // no change
    }

    if (doormsg.isEmergencyUnlock) {
        _soundEmergencyUnlock.playAsync();
    } else if (doormsg.isLockButton) {
        _soundLockButton.playAsync();
    } else if (doormsg.isUnlockButton) {
        if (msg.isOpen()) {
            _soundZonk.playAsync();
        } else {
            _soundUnlockButton.playAsync();
        }
    }

    _oldMessage = msg;
}

void MainWindow::_LED(const bool on)
{
    if (on)
        ui->LED->setPixmap(QPixmap(IMAGE_LED_GREEN));
    else
        ui->LED->setPixmap(QPixmap(IMAGE_LED_RED));
}
