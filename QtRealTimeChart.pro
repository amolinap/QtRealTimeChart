#-------------------------------------------------
#
# Project created by QtCreator 2019-02-22T08:03:16
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QtRealTimeChart
TEMPLATE = app

SOURCES += main.cpp \
    src/qchartviewer.cpp \
    src/realtimedemo.cpp \
    src/realtimedemozoomscroll.cpp \
    src/realtimezoomscroll.cpp \
    src/realtimetrack.cpp \
    src/QtChartWindow.cpp

HEADERS  += src/qchartviewer.h \
    src/realtimedemo.h \
    src/realtimedemozoomscroll.h \
    src/realtimezoomscroll.h \
    src/realtimetrack.h \
    src/QtChartWindow.h \
    src/QtConfiguration.h

FORMS  += src/QtChartWindow.ui

RESOURCES += realtimedemo.qrc

INCLUDEPATH += include \
    src

DEFINES += CHARTDIR_HIDE_OBSOLETE _CRT_SECURE_NO_WARNINGS

PROYDIR = C:\\Repositorios\\QtRealTimeChart

win32 {
    contains(QMAKE_HOST.arch, x86_64) {
        LIBS += $$PWD/lib64/chartdir60.lib
        #QMAKE_POST_LINK = copy /Y ..\\..\\lib64\\chartdir60.dll $(DESTDIR)
        QMAKE_POST_LINK = copy /Y \"$$PWD\\lib64\\chartdir60.dll\" $(DESTDIR)
    } else {
        LIBS += $$PWD/lib32/chartdir60.lib
        #QMAKE_POST_LINK = copy /Y ..\\..\\lib32\\chartdir60.dll $(DESTDIR)
        QMAKE_POST_LINK = copy /Y \"$$PWD\\lib32\\chartdir60.dll\" $(DESTDIR)
    }
}

macx:LIBS += -L$$PWD/lib -lchartdir
macx:QMAKE_POST_LINK += mkdir -p \"`dirname $(TARGET)`/../Frameworks\";
macx:QMAKE_POST_LINK += cp $$PWD/lib/libchartdir.6.dylib \"`dirname $(TARGET)`/../Frameworks\";
macx:QMAKE_POST_LINK += install_name_tool -change libchartdir.6.dylib \
     \@loader_path/../Frameworks/libchartdir.6.dylib \"$(TARGET)\";

unix:!macx:LIBS += -L$$PWD/lib -lchartdir
unix:!macx:QMAKE_RPATHDIR += $$PWD/lib
