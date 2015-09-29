#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>
#include <QHBoxLayout>

#include "qrwidget.h"

class MainWindow : public QMainWindow
{
public:
    MainWindow() :
        QMainWindow()
    {
        _qrWidget = new QRWidget;

        statusBar()->showMessage(tr("Boofar"));

        QHBoxLayout *layout = new QHBoxLayout;

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

    QRWidget* _qrWidget = { nullptr };
};

#endif
