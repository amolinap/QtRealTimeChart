#include "realtimedemo.h"
#include "realtimezoomscroll.h"
#include "realtimetrack.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyleSheet("* {font-family:arial;font-size:11px}");
    RealtimeTrack demo;
    demo.show();

    return app.exec();
}
