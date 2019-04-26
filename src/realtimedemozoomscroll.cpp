#include <QApplication>
#include <QButtonGroup>
#include <QIcon>
#include <QPushButton>
#include <QComboBox>
#include "realtimedemozoomscroll.h"
#include "chartdir.h"
#include <math.h>
#include <stdio.h>
#include <sstream>

using namespace std;

static const int DataInterval = 250;

RealtimeDemoZoomScroll::RealtimeDemoZoomScroll(QWidget *parent) :
    QDialog(parent)
{
    //
    // Set up the GUI
    //

    setFixedSize(772, 380);
    setWindowTitle("Simple Realtime Chart");

    // The frame on the left side
    QFrame *frame = new QFrame(this);
    frame->setGeometry(4, 4, 120, 277);
    frame->setFrameShape(QFrame::StyledPanel);

    // Pointer push button
    QPushButton *pointerPB = new QPushButton(QIcon(":/pointer.png"), "Pointer", frame);
    pointerPB->setGeometry(4, 8, 112, 28);
    pointerPB->setStyleSheet("QPushButton { text-align:left; padding:5px}");
    pointerPB->setCheckable(true);

    // Zoom In push button
    QPushButton *zoomInPB = new QPushButton(QIcon(":/zoomin.png"), "Zoom In", frame);
    zoomInPB->setGeometry(4, 36, 112, 28);
    zoomInPB->setStyleSheet("QPushButton { text-align:left; padding:5px}");
    zoomInPB->setCheckable(true);

    // Zoom Out push button
    QPushButton *zoomOutPB = new QPushButton(QIcon(":/zoomout.png"), "Zoom Out", frame);
    zoomOutPB->setGeometry(4, 64, 112, 28);
    zoomOutPB->setStyleSheet("QPushButton { text-align:left; padding:5px}");
    zoomOutPB->setCheckable(true);

    // Run push button
    QPushButton *runPB = new QPushButton(QIcon(":/play.png"), "Run", frame);
    runPB->setGeometry(4, 92, 112, 28);
    runPB->setStyleSheet("QPushButton { text-align:left; padding:5px}");
    runPB->setCheckable(true);

    // Freeze push button
    QPushButton *freezePB = new QPushButton(QIcon(":/pause.png"), "Freeze", frame);
    freezePB->setGeometry(4, 120, 112, 28);
    freezePB->setStyleSheet("QPushButton { text-align:left; padding:5px}");
    freezePB->setCheckable(true);

    // The Run/Freeze buttons form a button group
    QButtonGroup *runFreezeControl = new QButtonGroup(frame);
    runFreezeControl->addButton(runPB, 1);
    runFreezeControl->addButton(freezePB, 0);
    connect(runFreezeControl, SIGNAL(buttonPressed(int)), SLOT(onRunFreezeChanged(int)));

    QButtonGroup *mouseUsage = new QButtonGroup(frame);
    mouseUsage->addButton(pointerPB, Chart::MouseUsageScroll);
    mouseUsage->addButton(zoomInPB, Chart::MouseUsageZoomIn);
    mouseUsage->addButton(zoomOutPB, Chart::MouseUsageZoomOut);
    connect(mouseUsage, SIGNAL(buttonPressed(int)), SLOT(onMouseUsageChanged(int)));

    // Update Period drop down list box
    (new QLabel("Update Period (ms)", frame))->setGeometry(6, 80, 108, 16);
    QComboBox *updatePeriod = new QComboBox(frame);
    updatePeriod->setGeometry(6, 96, 108, 21);
    updatePeriod->addItems(QStringList() << "250" << "500" << "750" << "1000" << "1250" << "1500"
                           << "1750" << "2000");
    connect(updatePeriod, SIGNAL(currentIndexChanged(QString)),
            SLOT(onUpdatePeriodChanged(QString)));

    // Alpha Value display
    (new QLabel("Alpha", frame))->setGeometry(6, 200, 48, 21);
    m_ValueA = new QLabel(frame);
    m_ValueA->setGeometry(55, 200, 59, 21);
    m_ValueA->setFrameShape(QFrame::StyledPanel);

    // Beta Value display
    (new QLabel("Beta", frame))->setGeometry(6, 223, 48, 21);
    m_ValueB = new QLabel(frame);
    m_ValueB->setGeometry(55, 223, 59, 21);
    m_ValueB->setFrameShape(QFrame::StyledPanel);

    // Gamma Value display
    (new QLabel("Gamma", frame))->setGeometry(6, 246, 48, 21);
    m_ValueC = new QLabel(frame);
    m_ValueC->setGeometry(55, 246, 59, 21);
    m_ValueC->setFrameShape(QFrame::StyledPanel);

    // Chart Viewer
    m_ChartViewer = new QChartViewer(this);
    m_ChartViewer->setGeometry(132, 8, 640, 350);
    //m_ChartViewer->setMouseUsage(Chart::MouseUsageScroll);
    connect(m_ChartViewer, SIGNAL(viewPortChanged()), SLOT(onViewPortChanged()));
    connect(m_ChartViewer, SIGNAL(mouseMovePlotArea(QMouseEvent*)), SLOT(onMouseMovePlotArea(QMouseEvent*)));

    // Horizontal scroll bar
    m_HScrollBar = new QScrollBar(Qt::Horizontal, this);
    m_HScrollBar->setGeometry(128, 358, 640, 17);
    connect(m_HScrollBar, SIGNAL(valueChanged(int)), SLOT(onHScrollBarChanged(int)));

    // Enable mouse wheel zooming by setting the zoom ratio to 1.1 per wheel event
    m_ChartViewer->setMouseWheelZoomRatio(1.1);

    // Clear data arrays to Chart::NoValue
    //for (int i = 0; i < sampleSizeDemo; ++i)
    //    m_timeStamps[i] = m_dataSeriesA[i] = m_dataSeriesB[i] = m_dataSeriesC[i] = Chart::NoValue;
    m_currentIndex = 0;

    // Set m_nextDataTime to the current time. It is used by the real time random number
    // generator so it knows what timestamp should be used for the next data point.
    m_nextDataTime = QDateTime::currentDateTime();

    // Initially set the mouse to drag to scroll mode.
    pointerPB->click();

    // Set up the data acquisition mechanism. In this demo, we just use a timer to get a
    // sample every 250ms.
    QTimer *dataRateTimer = new QTimer(this);
    dataRateTimer->start(DataInterval);
    connect(dataRateTimer, SIGNAL(timeout()), SLOT(getData()));

    // Set up the chart update timer
    m_ChartUpdateTimer = new QTimer(this);
    connect(m_ChartUpdateTimer, SIGNAL(timeout()), SLOT(updateChart()));

    // Can start now
    updatePeriod->setCurrentIndex(3);
    runPB->click();
}

RealtimeDemoZoomScroll::~RealtimeDemoZoomScroll()
{
    delete m_ChartViewer->getChart();
}

//
// The Pointer, Zoom In or Zoom out button is pressed
//
void RealtimeDemoZoomScroll::onMouseUsageChanged(int mouseUsage)
{
    m_ChartViewer->setMouseUsage(mouseUsage);
}

void RealtimeDemoZoomScroll::onViewPortChanged()
{
    qDebug()<<"onViewPortChanged";

    // In addition to updating the chart, we may also need to update other controls that
    // changes based on the view port.
    updateControls(m_ChartViewer);

    // Update the chart if necessary
    if (m_ChartViewer->needUpdateChart())
    {
        drawChart(m_ChartViewer);
    }
}

void RealtimeDemoZoomScroll::updateControls(QChartViewer *viewer)
{
    // The logical length of the scrollbar. It can be any large value. The actual value does
    // not matter.
    const int scrollBarLen = 1000000000;

    // Update the horizontal scroll bar
    m_HScrollBar->setEnabled(viewer->getViewPortWidth() < 1);
    m_HScrollBar->setPageStep((int)ceil(viewer->getViewPortWidth() * scrollBarLen));
    m_HScrollBar->setSingleStep(min(scrollBarLen / 100, m_HScrollBar->pageStep()));
    m_HScrollBar->setRange(0, scrollBarLen - m_HScrollBar->pageStep());
    m_HScrollBar->setValue((int)(0.5 + viewer->getViewPortLeft() * scrollBarLen));
}

void RealtimeDemoZoomScroll::onMouseMovePlotArea(QMouseEvent *)
{
    trackLineLabel((XYChart *)m_ChartViewer->getChart(), m_ChartViewer->getPlotAreaMouseX());
    m_ChartViewer->updateDisplay();
}

void RealtimeDemoZoomScroll::onHScrollBarChanged(int value)
{
    qDebug()<<"onHScrollBarChanged";
    qDebug()<<m_ChartViewer->isInViewPortChangedEvent();

    if (!m_ChartViewer->isInViewPortChangedEvent())
    {
        qDebug()<<"isInViewPortChangedEvent";

        // Set the view port based on the scroll bar
        int scrollBarLen = m_HScrollBar->maximum() + m_HScrollBar->pageStep();
        m_ChartViewer->setViewPortLeft(value / (double)scrollBarLen);

        // Update the chart display without updating the image maps. (We can delay updating
        // the image map until scrolling is completed and the chart display is stable.)
        m_ChartViewer->updateViewPort(true, false);
    }
}

//
// A utility to shift a new data value into a data array
//
static void shiftData(double *data, int len, double newValue)
{
    memmove(data, data + 1, sizeof(*data) * (len - 1));
    data[len - 1] = newValue;
}

//
// Shift new data values into the real time data series
//
void RealtimeDemoZoomScroll::getData()
{
    QDateTime now = QDateTime::currentDateTime();

    do
    {
        double currentTime = Chart::chartTime2(m_nextDataTime.toTime_t()) + m_nextDataTime.time().msec() / 1000.0;

        double p = currentTime * 4;
        double dataA = 20 + cos(p * 129241) * 10 + 1 / (cos(p) * cos(p) + 0.01);
        double dataB = 150 + 100 * sin(p / 27.7) * sin(p / 10.1);
        double dataC = 150 + 100 * cos(p / 6.7) * cos(p / 11.9);

        valores[0].append(currentTime);
        valores[1].append(dataA);
        valores[2].append(dataB);
        valores[3].append(dataC);

        ++m_currentIndex;

        m_nextDataTime = m_nextDataTime.addMSecs(DataInterval);
    }
    while (m_nextDataTime < now);

    m_ValueA->setText(QString::number(valores[1].value(valores[1].count()-1), 'f', 2));
    m_ValueB->setText(QString::number(valores[2].value(valores[2].count()-1), 'f', 2));
    m_ValueC->setText(QString::number(valores[3].value(valores[3].count()-1), 'f', 2));

    qDebug()<<"Index: "<<m_currentIndex;
}

//
// Draw chart
//
void RealtimeDemoZoomScroll::drawChart(QChartViewer *viewer)
{
    XYChart *c = new XYChart(640, 350);

    c->setPlotArea(55, 50, c->getWidth() - 85, c->getHeight() - 80, c->linearGradientColor(0, 50, 0, c->getHeight() - 35, 0xf0f6ff, 0xa0c0ff), -1, Chart::Transparent, 0xffffff, 0xffffff);
    c->setClipping();
    c->addTitle("Field Intensity at Observation Satellite", "timesbi.ttf", 15)->setBackground(0xdddddd, 0x000000, Chart::glassEffect());

    LegendBox *b = c->addLegend(55, 25, false, "arialbd.ttf", 10);
    b->setBackground(Chart::Transparent);
    b->setLineStyleKey();

    c->xAxis()->setColors(Chart::Transparent);
    c->yAxis()->setColors(Chart::Transparent);
    c->xAxis()->setLabelStyle("arial.ttf", 10);
    c->yAxis()->setLabelStyle("arial.ttf", 10);
    c->yAxis()->setTickLength(0);
    c->yAxis()->setTitle("Ionic Temperature (C)", "arialbd.ttf", 12);

    LineLayer *layer = c->addLineLayer();
    layer->setLineWidth(2);
    layer->setFastLineMode();

    double lastTime = valores[0].at(m_currentIndex - 1);

    if (lastTime != Chart::NoValue)
    {
        c->xAxis()->setDateScale(lastTime - DataInterval * sampleSizeDemo / 1000, lastTime);

        qDebug()<<"isInMouseMoveEvent"<<lastTime - DataInterval * sampleSizeDemo / 1000;

        layer->setXData(DoubleArray(valores[0].constData(), m_currentIndex));
        layer->addDataSet(DoubleArray(valores[1].constData(), m_currentIndex), 0xff0000, "Alfa");
    }

    c->xAxis()->setTickDensity(75);
    c->yAxis()->setTickDensity(30);
    c->xAxis()->setFormatCondition("align", 3600);
    c->xAxis()->setMultiFormat(Chart::StartOfDayFilter(), "<*font=bold*>{value|hh:nn<*br*>mmm dd}", Chart::AllPassFilter(), "{value|hh:nn}");
    c->xAxis()->setFormatCondition("align", 60);
    c->xAxis()->setLabelFormat("{value|hh:nn}");
    c->xAxis()->setFormatCondition("else");
    c->xAxis()->setLabelFormat("{value|hh:nn:ss}");
    c->xAxis()->setMinTickInc(1);

    if (!viewer->isInMouseMoveEvent())
    {
        trackLineLabel(c, (0 == viewer->getChart()) ? c->getPlotArea()->getRightX() : viewer->getPlotAreaMouseX());

        qDebug()<<"isInMouseMoveEvent";
    }

    delete viewer->getChart();

    viewer->setChart(c);
}

//
// The Run or Freeze button is pressed
//
void RealtimeDemoZoomScroll::onRunFreezeChanged(int run)
{
    if (run)
        m_ChartUpdateTimer->start();
    else
        m_ChartUpdateTimer->stop();
}

//
// User changes the chart update period
//
void RealtimeDemoZoomScroll::onUpdatePeriodChanged(QString text)
{
    m_ChartUpdateTimer->start(text.toInt());
}

//
// Update the chart. Instead of drawing the chart directly, we call updateViewPort, which
// will trigger a ViewPortChanged signal. We update the chart in the signal handler
// "drawChart". This can take advantage of the built-in rate control in QChartViewer to
// ensure a smooth user interface, even for extremely high update rate. (See the
// documentation on QChartViewer.setUpdateInterval).
//
void RealtimeDemoZoomScroll::updateChart()
{
    //m_ChartViewer->updateViewPort(true, false);

    if (m_currentIndex > 0)
    {
        //
        // As we added more data, we may need to update the full range of the viewport.
        //

        double startDate = valores[0].at(0);
        double endDate = valores[0].at(m_currentIndex - 1);

        // Use the initialFullRange (which is 60 seconds in this demo) if this is sufficient.
        double duration = endDate - startDate;
        if (duration < 60)
            endDate = startDate + 60;

        // Update the full range to reflect the actual duration of the data. In this case,
        // if the view port is viewing the latest data, we will scroll the view port as new
        // data are added. If the view port is viewing historical data, we would keep the
        // axis scale unchanged to keep the chart stable.
        int updateType = Chart::ScrollWithMax;
        if (m_ChartViewer->getViewPortLeft() + m_ChartViewer->getViewPortWidth() < 0.999)
            updateType = Chart::KeepVisibleRange;
        bool scaleHasChanged = m_ChartViewer->updateFullRangeH("x", startDate, endDate, updateType);

        qDebug()<<"updateChart";

        // Set the zoom in limit as a ratio to the full range
        m_ChartViewer->setZoomInWidthLimit(10 / (m_ChartViewer->getValueAtViewPort("x", 1) -
                                                 m_ChartViewer->getValueAtViewPort("x", 0)));

        // Trigger the viewPortChanged event to update the display if the axis scale has changed
        // or if new data are added to the existing axis scale.
        if (scaleHasChanged || (duration < 60))
            m_ChartViewer->updateViewPort(true, false);
    }
}

void RealtimeDemoZoomScroll::trackLineLabel(XYChart *c, int mouseX)
{
    // Clear the current dynamic layer and get the DrawArea object to draw on it.
    DrawArea *d = c->initDynamicLayer();

    // The plot area object
    PlotArea *plotArea = c->getPlotArea();

    // Get the data x-value that is nearest to the mouse, and find its pixel coordinate.
    double xValue = c->getNearestXValue(mouseX);
    int xCoor = c->getXCoor(xValue);
    if (xCoor < plotArea->getLeftX())
        return;

    // Draw a vertical track line at the x-position
    d->vline(plotArea->getTopY(), plotArea->getBottomY(), xCoor, 0x888888);

    // Draw a label on the x-axis to show the track line position.
    ostringstream xlabel;
    xlabel << "<*font,bgColor=000000*> " << c->xAxis()->getFormattedLabel(xValue, "hh:nn:ss.ff")
           << " <*/font*>";
    TTFText *t = d->text(xlabel.str().c_str(), "arialbd.ttf", 10);

    // Restrict the x-pixel position of the label to make sure it stays inside the chart image.
    int xLabelPos = max(0, min(xCoor - t->getWidth() / 2, c->getWidth() - t->getWidth()));
    t->draw(xLabelPos, plotArea->getBottomY() + 6, 0xffffff);
    t->destroy();

    // Iterate through all layers to draw the data labels
    for (int i = 0; i < c->getLayerCount(); ++i) {
        Layer *layer = c->getLayerByZ(i);

        // The data array index of the x-value
        int xIndex = layer->getXIndexOf(xValue);

        // Iterate through all the data sets in the layer
        for (int j = 0; j < layer->getDataSetCount(); ++j)
        {
            DataSet *dataSet = layer->getDataSetByZ(j);
            const char *dataSetName = dataSet->getDataName();

            // Get the color, name and position of the data label
            int color = dataSet->getDataColor();
            int yCoor = c->getYCoor(dataSet->getPosition(xIndex), dataSet->getUseYAxis());

            // Draw a track dot with a label next to it for visible data points in the plot area
            if ((yCoor >= plotArea->getTopY()) && (yCoor <= plotArea->getBottomY()) && (color !=
                                                                                        Chart::Transparent) && dataSetName && *dataSetName)
            {
                d->circle(xCoor, yCoor, 4, 4, color, color);

                ostringstream label;
                label << "<*font,bgColor=" << hex << color << "*> "
                      << c->formatValue(dataSet->getValue(xIndex), "{value|P4}") << " <*font*>";
                t = d->text(label.str().c_str(), "arialbd.ttf", 10);

                // Draw the label on the right side of the dot if the mouse is on the left side the
                // chart, and vice versa. This ensures the label will not go outside the chart image.
                if (xCoor <= (plotArea->getLeftX() + plotArea->getRightX()) / 2)
                    t->draw(xCoor + 6, yCoor, 0xffffff, Chart::Left);
                else
                    t->draw(xCoor - 6, yCoor, 0xffffff, Chart::Right);

                t->destroy();
            }
        }
    }
}
