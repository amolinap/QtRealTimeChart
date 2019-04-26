#include <QApplication>

#include "realtimedemo.h"
#include "realtimedemozoomscroll.h"
#include "realtimezoomscroll.h"
#include "realtimetrack.h"
#include "QtChartWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyleSheet("* {font-family:arial;font-size:11px}");
    //RealtimeDemo demo;
    //RealtimeDemoZoomScroll demo;
    //RealtimeTrack demo;
    //RealTimeZoomScroll demo;
    QtChartWindow demo;
    demo.show();

    return app.exec();
}
