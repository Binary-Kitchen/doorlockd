#include <exception>
#include <memory>

#include <QPainter>
#include <QDebug>

#include <qrencode.h>

#include "qrwidget.h"

QRWidget::QRWidget(QWidget* parent) :
    QWidget(parent),
    _data(" ")
{
}

void QRWidget::setQRData(const std::string &data)
{
    _data = data;
    update();
}

void QRWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    std::unique_ptr<QRcode, void(*)(QRcode*)> qr(
                QRcode_encodeString(_data.c_str(), 1, QR_ECLEVEL_L, QR_MODE_8, 1),
                [] (QRcode* ptr) {
        if (ptr)
            QRcode_free(ptr);
    });

    if (!qr)
        throw std::runtime_error("Error generating QR Code");

    QColor fg("black");
    QColor bg("white");

    painter.setBrush(bg);
    painter.setPen(Qt::NoPen);
    painter.drawRect(0,0,width(),height());
    painter.setBrush(fg);

    const int s=qr->width>0?qr->width:1;
    const double w=width();
    const double h=height();
    const double aspect=w/h;
    const double scale=((aspect>1.0)?h:w)/s;

    for(int y=0;y<s;y++) {
        const int yy=y*s;
        for(int x=0;x<s;x++) {
            const int xx=yy+x;
            const unsigned char b=qr->data[xx];
            if(b &0x01) {
                const double rx1=x*scale, ry1=y*scale;
                QRectF r(rx1, ry1, scale, scale);
                painter.drawRects(&r,1);
            }
        }
    }
}
