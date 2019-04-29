#ifndef QTCHARTWINDOW_H
#define QTCHARTWINDOW_H

#include <QWidget>
#include <QDialog>
#include <QDateTime>
#include <QTimer>
#include <QLabel>
#include <QScrollBar>
#include <QDebug>
#include <QFileDialog>
#include <QCheckBox>
#include <QGridLayout>
#include <QMap>
#include <QVector>

#include <math.h>
#include <vector>
#include <sstream>

#include "qchartviewer.h"
#include "chartdir.h"


namespace Ui
{
    class QtChartWindow;
}

class QtChartWindow : public QWidget {
    Q_OBJECT

public:
    QtChartWindow(QWidget* parent = 0);
    ~QtChartWindow();

protected:
    void resizeEvent(QResizeEvent *event);

private:
    Ui::QtChartWindow* ui;

    static const int sampleSize = 10000;
    static const int initialFullRange = 60;
    static const int zoomInLimit = 10;

    QMap<int, QVector<double> > dataSeries; // The values for the data series

    quint32 m_currentIndex;                 // Index of the array position to which new values are added.

    QDateTime m_nextDataTime;           // Used by the random number generator to generate realtime data.

    QChartViewer *m_ChartViewer;        // QChartViewer control
    QTimer *m_ChartUpdateTimer;         // The chart update timer
    QScrollBar *m_HScrollBar;           // The scroll bar

    void drawChart(QChartViewer *viewer);           // Draw chart
    void trackLineLabel(XYChart *c, int mouseX);    // Draw track cursor
    void updateControls(QChartViewer *viewer);      // Update other controls as viewport changes

    QList<double> valuesList;
    int widthPlot, heightPlot;
    QWidget* curvesWidget;                ///< The QWidget containing the curve selection button
    QGridLayout* curvesWidgetLayout;      ///< The layout for the curvesWidget QWidget
    QCheckBox* selectAllCheckBox;
    QMap<QString, QLabel*>* curveLabels;  ///< References to the curve labels
    QMap<QString, int> intData;           ///< Current values for integer-valued curves

private slots:
    void onMouseUsageChanged(int mouseUsage);       // Pointer/zoom in/zoom out button clicked
    void onSave(bool);                              // Save button clicked
    void getData();                                 // Get new data values
    void onUpdatePeriodChanged(QString);            // The chart update timer interval has changed.
    void onMouseMovePlotArea(QMouseEvent *event);   // Mouse move on plot area
    void onChartUpdateTimer();                      // Update the chart.
    void onViewPortChanged();                       // Viewport has changed
    void onHScrollBarChanged(int value);            // Scrollbar changed

    void addItem(const QString name);
    void refresh();

};

#endif
