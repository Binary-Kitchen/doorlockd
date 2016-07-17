#include "config.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindow),
    _soundLock(Wave(SOUND_LOCK)),
    _soundUnlock(Wave(SOUND_UNLOCK)),
    _soundEmergencyUnlock(Wave(SOUND_EMERGENCY_UNLOCK)),
    _soundZonk(Wave(SOUND_ZONK)),
    _soundLockButton(Wave(SOUND_LOCK_BUTTON)),
    _soundUnlockButton(Wave(SOUND_UNLOCK_BUTTON))
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
    ui->qrwidget->setQRData(msg.web_address());
    ui->address->setText(QString::fromStdString(msg.web_address()));
    ui->token->setText(QString::fromStdString(msg.token()));
    QString statusMessage("");

    const auto &doormsg = msg.doormessage();

    _LED(msg.isOpen());

    if (_oldMessage.isOpen()
        && !msg.isOpen()
        && !doormsg.isLockButton) {
        // regular close
        statusMessage = "Bye bye. See you next time!";
        _soundLock.playAsync();
    } else if (!_oldMessage.isOpen() && msg.isOpen()) {
        // regular open
        statusMessage = "Come in! Happy hacking!";
        _soundUnlock.playAsync();
    } else {
        // no change
    }

    if (doormsg.isEmergencyUnlock) {
        _soundEmergencyUnlock.playAsync();
        statusMessage = "!! EMERGENCY UNLOCK !!";
    } else if (doormsg.isLockButton) {
        _soundLockButton.playAsync();
        statusMessage = "!! LOCK BUTTON !!";
    } else if (doormsg.isUnlockButton) {
        statusMessage = "!! UNLOCK BUTTON !!";
        if (msg.isOpen()) {
            _soundZonk.playAsync();
        } else {
            _soundUnlockButton.playAsync();
        }
    }

    ui->message->setText(statusMessage);

    _oldMessage = msg;
}

void MainWindow::_LED(const bool on)
{
    if (on)
        ui->LED->setPixmap(QPixmap(IMAGE_LED_GREEN));
    else
        ui->LED->setPixmap(QPixmap(IMAGE_LED_RED));
}
