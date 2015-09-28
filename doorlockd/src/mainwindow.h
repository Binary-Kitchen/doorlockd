#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>

#include "qrwidget.h"

class MainWindow : public QWidget
{
public:
    MainWindow() :
        QWidget()
    {
        _qrWidget = new QRWidget;

        _layout = new QVBoxLayout;
        _layout->addWidget(_qrWidget);

        _label = new QLabel;
        _label->setText("Hello, world!");

        QFont font("Times", 25, QFont::Bold);
        _label->setFont(font);
        _layout->addWidget(_label);

        setLayout(_layout);
        showFullScreen();
    }

    MainWindow(const MainWindow&);
    MainWindow& operator =(const MainWindow&);

    void setQRCode(const QString &str)
    {
        _qrWidget->setQRData(str);
    }

    void setLabel(const QString &str)
    {
        _label->setText(str);
    }

private:

    QRWidget* _qrWidget = { nullptr };
    QVBoxLayout* _layout = { nullptr };
    QLabel* _label = { nullptr };

};

#endif
