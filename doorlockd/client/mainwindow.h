#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QStatusBar>

#include "qrwidget.h"

class MainWindow : public QMainWindow
{
public:
    MainWindow() :
        QMainWindow()
    {
        layout = new QHBoxLayout;

        _qrWidget = new QRWidget;
        layout->addWidget(_qrWidget);

        QWidget* window = new QWidget;
        window->setLayout(layout);

        setCentralWidget(window);
    }

    MainWindow(const MainWindow&);
    MainWindow& operator =(const MainWindow&);

    void setQRCode(const std::string &str)
    {
        _qrWidget->setQRData(str);
    }

private:

    QHBoxLayout* layout = { nullptr };
    QRWidget* _qrWidget = { nullptr };
};

#endif
