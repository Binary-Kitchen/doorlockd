#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

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

    void setQRCode(const std::string &token);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
