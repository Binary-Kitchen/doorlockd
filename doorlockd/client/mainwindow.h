#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include "../lib/clientmessage.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);

    MainWindow(const MainWindow &rhs);
    MainWindow &operator =(const MainWindow &rhs);

    ~MainWindow();

    void setClientmessage(const Clientmessage &msg);

private:
    Ui::MainWindow *ui;

    void _LED(const bool on);
};

#endif // MAINWINDOW_H
