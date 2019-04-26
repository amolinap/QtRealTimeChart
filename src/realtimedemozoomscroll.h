#ifndef REALTIMEDEMOZOOMSCROLL_H
#define REALTIMEDEMOZOOMSCROLL_H

#include <QDialog>
#include <QDateTime>
#include <QTimer>
#include <QLabel>
#include <QScrollBar>

#include "qchartviewer.h"

// The number of samples per data series used in this demo
const int sampleSizeDemo = 240;

class RealtimeDemoZoomScroll : public QDialog {
    Q_OBJECT
public:
    RealtimeDemoZoomScroll(QWidget *parent = 0);
    ~RealtimeDemoZoomScroll();

private:
    double m_timeStamps[sampleSizeDemo];	// The timestamps for the data series
    double m_dataSeriesA[sampleSizeDemo];	// The values for the data series A
    double m_dataSeriesB[sampleSizeDemo];	// The values for the data series B
    double m_dataSeriesC[sampleSizeDemo];	// The values for the data series C

    quint32 m_currentIndex;                 // Index of the array position to which new values are added.

    QDateTime m_nextDataTime;           // Used by the random number generator to generate realtime data.

    QLabel *m_ValueA;                   // Label to display the realtime value A
    QLabel *m_ValueB;                   // Label to display the realtime value B
    QLabel *m_ValueC;                   // Label to display the realtime value C

    QChartViewer *m_ChartViewer;        // QChartViewer control
    QTimer *m_ChartUpdateTimer;         // The chart update timer

    QScrollBar *m_HScrollBar;           // The scroll bar

private slots:
    void onRunFreezeChanged(int);       // The "Run" or "Freeze" button has been pressed
    void onUpdatePeriodChanged(QString);// The chart update timer interval has changed.
    void getData();                     // Get new data values
    void updateChart();                 // Update the chart.
    void drawChart(QChartViewer *viewer);                   // Draw the chart.
    void onViewPortChanged();
    void onMouseMovePlotArea(QMouseEvent *);
    void trackLineLabel(XYChart *c, int mouseX);
    void onHScrollBarChanged(int value);
    void updateControls(QChartViewer *viewer);
};

#endif // REALTIMEDEMO_H
