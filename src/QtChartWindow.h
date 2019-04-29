/**
 *	@file QtCommunications.h
 * 	@brief Clase QtCommunications
 *
 *	@author Alejandro Molina Pulido <am.alex09@gmail.com>
 */
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

/**
 * @brief Clase QtSerialPortC
 *
 * Este widget representa la ventana de configuración de la comunicación
 */
class QtChartWindow : public QWidget {
    Q_OBJECT

public:
    /** @brief Constructor de la clase
    *
    * @param mavlink Objeto MAVLink
    * @param parent El padre de la clase */
    QtChartWindow(QWidget* parent = 0);
    ~QtChartWindow();

protected:
    void resizeEvent(QResizeEvent *event);

private:
    Ui::QtChartWindow* ui;

    // The number of samples per data series used in this demo
    static const int sampleSize = 10000;

    // The initial full range is set to 60 seconds of data.
    static const int initialFullRange = 60;

    // The maximum zoom in is 10 seconds.
    static const int zoomInLimit = 10;

    //double m_timeStamps[sampleSize];	// The timestamps for the data series
    /*double m_dataSeriesA[sampleSize];	// The values for the data series A
    double m_dataSeriesB[sampleSize];	// The values for the data series B
    double m_dataSeriesC[sampleSize];	// The values for the data series C

    double m_dataSeriesAlt[sampleSize];	// The values for the data series C
    double m_dataSeriesIAS[sampleSize];	// The values for the data series C
    double m_dataSeriesSpee[sampleSize];	// The values for the data series C*/
    QMap<int, QVector<double> > dataSeries;

    //QVector<QVector<double>> dataSeries;

    quint32 m_currentIndex;                 // Index of the array position to which new values are added.

    QDateTime m_nextDataTime;           // Used by the random number generator to generate realtime data.

    QLabel *m_ValueA;                   // Label to display the realtime value A
    QLabel *m_ValueB;                   // Label to display the realtime value B
    QLabel *m_ValueC;                   // Label to display the realtime value C

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
    void getData();                     // Get new data values
    void onUpdatePeriodChanged(QString);            // The chart update timer interval has changed.
    void onMouseMovePlotArea(QMouseEvent *event);   // Mouse move on plot area
    void onChartUpdateTimer();                      // Update the chart.
    void onViewPortChanged();                       // Viewport has changed
    void onHScrollBarChanged(int value);            // Scrollbar changed

    void addItem(const QString name);
    void refresh();

};

#endif // QTCOMMUNICATIONS_H
