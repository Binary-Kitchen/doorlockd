#ifndef QRWIDGET_H
#define QRWIDGET_H

#include <QWidget>

class QRWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QRWidget(QWidget *parent = nullptr);
    void setQRData(const std::string &data);

private:
    QString _data;

protected:
    void paintEvent(QPaintEvent *);
};

#endif
