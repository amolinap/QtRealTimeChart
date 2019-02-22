#include "realtimedemo.h"
#include "realtimezoomscroll.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyleSheet("* {font-family:arial;font-size:11px}");
    RealTimeZoomScroll demo;
    demo.show();

    return app.exec();
}
