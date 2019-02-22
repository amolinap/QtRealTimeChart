#include "realtimedemo.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyleSheet("* {font-family:arial;font-size:11px}");
    RealtimeDemo demo;
    demo.show();

    return app.exec();
}
