#include "QtChartWindow.h"
#include "ui_QtChartWindow.h"

using namespace std;

static const int DataInterval = 250;

QtChartWindow::QtChartWindow(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::QtChartWindow)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
    setWindowTitle(tr("Realtime Chart with Zoom/Scroll and Track Line"));

    m_ChartViewer = new QChartViewer(this);
    m_ChartViewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //m_ChartViewer->setGeometry(128, 4, 640, 350);

    // Enable mouse wheel zooming by setting the zoom ratio to 1.1 per wheel event
    m_ChartViewer->setMouseWheelZoomRatio(1.1);

    connect(m_ChartViewer, SIGNAL(viewPortChanged()), SLOT(onViewPortChanged()));
    connect(m_ChartViewer, SIGNAL(mouseMovePlotArea(QMouseEvent*)), SLOT(onMouseMovePlotArea(QMouseEvent*)));

    // Horizontal scroll bar
    m_HScrollBar = new QScrollBar(Qt::Horizontal, m_ChartViewer);
    //m_HScrollBar->setGeometry(128, 358, 640, 17);
    connect(m_HScrollBar, SIGNAL(valueChanged(int)), SLOT(onHScrollBarChanged(int)));

    ui->verticalLayout_2->setSpacing(4);
    ui->verticalLayout_2->setMargin(2);
    ui->verticalLayout_2->addWidget(m_ChartViewer);
    ui->verticalLayout_2->addWidget(m_HScrollBar);

    // Clear data arrays to Chart::NoValue
    //for (int i = 0; i < sampleSize; ++i)
    //	m_timeStamps[i] = m_dataSeriesA[i] = m_dataSeriesB[i] = m_dataSeriesC[i] = Chart::NoValue;
    m_currentIndex = 0;

    m_nextDataTime = QDateTime::currentDateTime();
    // Set up the data acquisition mechanism. In this demo, we just use a timer to get a
    // sample every 250ms.
    QTimer *dataRateTimer = new QTimer(this);
    dataRateTimer->start(DataInterval);
    connect(dataRateTimer, SIGNAL(timeout()), SLOT(getData()));

    // Set up the chart update timer
    m_ChartUpdateTimer = new QTimer(this);
    connect(m_ChartUpdateTimer, SIGNAL(timeout()), SLOT(onChartUpdateTimer()));

    // Can start now
    //updatePeriod->setCurrentIndex(3);
    m_ChartUpdateTimer->start();

    curveLabels = new QMap<QString, QLabel*>();

    curvesWidget = new QWidget(ui->curveListWidget);
    ui->curveListWidget->setWidget(curvesWidget);
    curvesWidgetLayout = new QGridLayout(curvesWidget);
    curvesWidgetLayout->setMargin(2);
    curvesWidgetLayout->setSpacing(4);
    curvesWidgetLayout->setAlignment(Qt::AlignTop);

    curvesWidgetLayout->setColumnStretch(0, 0);
    curvesWidgetLayout->setColumnStretch(1, 0);
    curvesWidgetLayout->setColumnStretch(2, 80);
    curvesWidgetLayout->setColumnStretch(3, 50);

    curvesWidget->setLayout(curvesWidgetLayout);

    // Create curve list headings
    QLabel* label;
    QLabel* value;

    int labelRow = curvesWidgetLayout->rowCount();

    selectAllCheckBox = new QCheckBox("", this);
    connect(selectAllCheckBox, SIGNAL(clicked(bool)), this, SLOT(selectAllCurves(bool)));
    curvesWidgetLayout->addWidget(selectAllCheckBox, labelRow, 0, 1, 2);

    label = new QLabel(this);
    label->setText(tr("Name"));
    curvesWidgetLayout->addWidget(label, labelRow, 2);

    // Value
    value = new QLabel(this);
    value->setText(tr("Value"));
    curvesWidgetLayout->addWidget(value, labelRow, 3);

    /*QTimer* updateTimer = new QTimer();
    updateTimer->setInterval(300);
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(refresh()));
    updateTimer->start();*/
}

QtChartWindow::~QtChartWindow()
{
}

void QtChartWindow::resizeEvent(QResizeEvent *event)
{
    // Create an XYChart object of size 640 x 350 pixels
    //c->setSize(ui->gbChartPlot->width(), ui->gbChartPlot->height());
    widthPlot = ui->gbChartPlot->width() - 20;
    heightPlot = ui->gbChartPlot->height() - 70;
}

void QtChartWindow::addItem(const QString name)
{
    // Create curve list headings
    QLabel* label;
    QLabel* value;

    int labelRow = curvesWidgetLayout->rowCount();

    selectAllCheckBox = new QCheckBox("", this);
    connect(selectAllCheckBox, SIGNAL(clicked(bool)), this, SLOT(selectAllCurves(bool)));
    curvesWidgetLayout->addWidget(selectAllCheckBox, labelRow, 0, 1, 2);

    label = new QLabel(this);
    label->setText(name);
    curvesWidgetLayout->addWidget(label, labelRow, 2);

    // Value
    value = new QLabel(this);
    value->setText(tr("Value"));
    curvesWidgetLayout->addWidget(value, labelRow, 3);

    curveLabels->insert(name, value);
}

void QtChartWindow::refresh()
{
    QString str;
    QMap<QString, QLabel*>::iterator i;

    for (i = curveLabels->begin(); i != curveLabels->end(); ++i)
    {
        if (intData.contains(i.key()))
        {
            str.sprintf("% 11i", intData.value(i.key()));

            // Value
            i.value()->setText(str);
        }
    }
}
//
// The Pointer, Zoom In or Zoom out button is pressed
//
void QtChartWindow::onMouseUsageChanged(int mouseUsage)
{
    m_ChartViewer->setMouseUsage(mouseUsage);
}

//
// The Save button is pressed
//
void QtChartWindow::onSave(bool)
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save", "chartdirector_demo",
        "PNG (*.png);;JPG (*.jpg);;GIF (*.gif);;BMP (*.bmp);;SVG (*.svg);;PDF (*.pdf)");

    if (!fileName.isEmpty())
    {
        // Save the chart
        BaseChart *c = m_ChartViewer->getChart();
        if (0 != c)
            c->makeChart(fileName.toUtf8().constData());
    }
}

//
// User changes the chart update period
//
void QtChartWindow::onUpdatePeriodChanged(QString text)
{
    m_ChartUpdateTimer->start(text.toInt());
}

//
// Update the chart and the viewport periodically
//
void QtChartWindow::onChartUpdateTimer()
{
    if (m_currentIndex > 0)
    {
        //
        // As we added more data, we may need to update the full range of the viewport.
        //

        double startDate = dataSeries[0].at(0);
        double endDate = dataSeries[0].at(dataSeries[0].count()-1);

        // Use the initialFullRange (which is 60 seconds in this demo) if this is sufficient.
        double duration = endDate - startDate;
        if (duration < initialFullRange)
            endDate = startDate + initialFullRange;

        // Update the full range to reflect the actual duration of the data. In this case,
        // if the view port is viewing the latest data, we will scroll the view port as new
        // data are added. If the view port is viewing historical data, we would keep the
        // axis scale unchanged to keep the chart stable.
        int updateType = Chart::ScrollWithMax;
        if (m_ChartViewer->getViewPortLeft() + m_ChartViewer->getViewPortWidth() < 0.999)
            updateType = Chart::KeepVisibleRange;
        bool scaleHasChanged = m_ChartViewer->updateFullRangeH("x", startDate, endDate, updateType);

        // Set the zoom in limit as a ratio to the full range
        m_ChartViewer->setZoomInWidthLimit(zoomInLimit / (m_ChartViewer->getValueAtViewPort("x", 1) -
            m_ChartViewer->getValueAtViewPort("x", 0)));

        // Trigger the viewPortChanged event to update the display if the axis scale has changed
        // or if new data are added to the existing axis scale.
        if (scaleHasChanged || (duration < initialFullRange))
            m_ChartViewer->updateViewPort(true, false);
    }
}

//
// View port changed event
//
void QtChartWindow::onViewPortChanged()
{
    // In addition to updating the chart, we may also need to update other controls that
    // changes based on the view port.
    updateControls(m_ChartViewer);

    // Update the chart if necessary
    if (m_ChartViewer->needUpdateChart())
        drawChart(m_ChartViewer);
}

//
// User clicks on the the horizontal scroll bar
//
void QtChartWindow::onHScrollBarChanged(int value)
{
    if (!m_ChartViewer->isInViewPortChangedEvent())
    {
        // Set the view port based on the scroll bar
        int scrollBarLen = m_HScrollBar->maximum() + m_HScrollBar->pageStep();
        m_ChartViewer->setViewPortLeft(value / (double)scrollBarLen);

        // Update the chart display without updating the image maps. (We can delay updating
        // the image map until scrolling is completed and the chart display is stable.)
        m_ChartViewer->updateViewPort(true, false);
    }
}

//
// Update controls in the user interface when the view port changed
//
void QtChartWindow::updateControls(QChartViewer *viewer)
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

void QtChartWindow::getData()
{
    QDateTime now = QDateTime::currentDateTime();

    do
    {
        double currentTime = Chart::chartTime2(m_nextDataTime.toTime_t()) + m_nextDataTime.time().msec() / 1000.0;

        double p = currentTime * 4;
        double dataA = 20 + cos(p * 129241) * 10 + 1 / (cos(p) * cos(p) + 0.01);
        double dataB = 150 + 100 * sin(p / 27.7) * sin(p / 10.1);
        double dataC = 150 + 100 * cos(p / 6.7) * cos(p / 11.9);

        dataSeries[0].append(currentTime);
        dataSeries[1].append(dataA);
        dataSeries[2].append(dataB);
        dataSeries[3].append(dataC);

        ++m_currentIndex;

        m_nextDataTime = m_nextDataTime.addMSecs(DataInterval);
    }
    while (m_nextDataTime < now);

    /*m_ValueA->setText(QString::number(dataSeries[1].value(dataSeries[1].count()-1), 'f', 2));
    m_ValueB->setText(QString::number(dataSeries[2].value(dataSeries[2].count()-1), 'f', 2));
    m_ValueC->setText(QString::number(dataSeries[3].value(dataSeries[3].count()-1), 'f', 2));*/

    qDebug()<<"Index: "<<m_currentIndex;
}

//
// Draw chart
//
void QtChartWindow::drawChart(QChartViewer *viewer)
{
    // Get the start date and end date that are visible on the chart.
    double viewPortStartDate = viewer->getValueAtViewPort("x", viewer->getViewPortLeft());
    double viewPortEndDate = viewer->getValueAtViewPort("x", viewer->getViewPortLeft() +
        viewer->getViewPortWidth());

    // Extract the part of the data arrays that are visible.
    DoubleArray viewPortTimeStamps;
    DoubleArray viewPortDataSeriesA;
    DoubleArray viewPortDataSeriesB;
    DoubleArray viewPortDataSeriesC;

    DoubleArray viewPortDataSeriesAlt;
    DoubleArray viewPortDataSeriesIAS;
    DoubleArray viewPortDataSeriesSpee;

    if (m_currentIndex > 0)
    {
        // Get the array indexes that corresponds to the visible start and end dates
        int startIndex = (int)floor(Chart::bSearch(DoubleArray(dataSeries[0].constData(), m_currentIndex), viewPortStartDate));
        int endIndex = (int)ceil(Chart::bSearch(DoubleArray(dataSeries[0].constData(), m_currentIndex), viewPortEndDate));
        int noOfPoints = endIndex - startIndex + 1;

        // Extract the visible data
        viewPortTimeStamps = DoubleArray(dataSeries[0].constData() + startIndex, noOfPoints);
        viewPortDataSeriesA = DoubleArray(dataSeries[1].constData() + startIndex, noOfPoints);
        viewPortDataSeriesB = DoubleArray(dataSeries[2].constData() + startIndex, noOfPoints);
    }

    //
    // At this stage, we have extracted the visible data. We can use those data to plot the chart.
    //

    //================================================================================
    // Configure overall chart appearance.
    //================================================================================

    // Create an XYChart object of size 640 x 350 pixels
    //XYChart *c = new XYChart(840, 650);
    XYChart *c = new XYChart(widthPlot, heightPlot);

    // Set the plotarea at (55, 50) with width 80 pixels less than chart width, and height 80 pixels
    // less than chart height. Use a vertical gradient from light blue (f0f6ff) to sky blue (a0c0ff)
    // as background. Set border to transparent and grid lines to white (ffffff).
    c->setPlotArea(55, 50, c->getWidth() - 85, c->getHeight() - 80, c->linearGradientColor(0, 50, 0,
        c->getHeight() - 35, 0xf0f6ff, 0xa0c0ff), -1, Chart::Transparent, 0xffffff, 0xffffff);

    // As the data can lie outside the plotarea in a zoomed chart, we need enable clipping.
    c->setClipping();

    // Add a title to the chart using 18pt Arial font
    c->addTitle("    Realtime Chart with Zoom/Scroll and Track Line", "arial.ttf", 18);

    // Add a legend box at (55, 25) using horizontal layout. Use 10pt Arial Bold as font. Set the
    // background and border color to transparent and use line style legend key.
    LegendBox *b = c->addLegend(55, 25, false, "arialbd.ttf", 10);
    b->setBackground(Chart::Transparent);
    b->setLineStyleKey();

    // Set the x and y axis stems to transparent and the label font to 10pt Arial
    c->xAxis()->setColors(Chart::Transparent);
    c->yAxis()->setColors(Chart::Transparent);
    c->xAxis()->setLabelStyle("arial.ttf", 10);
    c->yAxis()->setLabelStyle("arial.ttf", 10);

    // Set the y-axis tick length to 0 to disable the tick and put the labels closer to the axis.
    c->yAxis()->setTickLength(0);

    // Add axis title using 12pt Arial Bold Italic font
    c->yAxis()->setTitle("Ionic Temperature (C)", "arialbd.ttf", 12);

    //================================================================================
    // Add data to chart
    //================================================================================

    //
    // In this example, we represent the data by lines. You may modify the code below to use other
    // representations (areas, scatter plot, etc).
    //

    // Add a line layer for the lines, using a line width of 2 pixels
    LineLayer *layer = c->addLineLayer();
    layer->setLineWidth(2);
    layer->setFastLineMode();

    // Now we add the 3 data series to a line layer, using the color red (ff0000), green (00cc00)
    // and blue (0000ff)
    layer->setXData(viewPortTimeStamps);
    layer->addDataSet(viewPortDataSeriesA, 0xff0000, "Alpha");
    layer->addDataSet(viewPortDataSeriesB, 0x00cc00, "Beta");
    layer->addDataSet(viewPortDataSeriesC, 0x0000ff, "Gamma");

    layer->addDataSet(viewPortDataSeriesAlt, 0x0000ff, "Altitude");
    layer->addDataSet(viewPortDataSeriesIAS, 0x0000ff, "IAS");
    layer->addDataSet(viewPortDataSeriesSpee, 0x0000ff, "Speed");

    //================================================================================
    // Configure axis scale and labelling
    //================================================================================

    // Set the x-axis as a date/time axis with the scale according to the view port x range.
    if (m_currentIndex > 0)
        c->xAxis()->setDateScale(viewPortStartDate, viewPortEndDate);

    // For the automatic axis labels, set the minimum spacing to 75/30 pixels for the x/y axis.
    c->xAxis()->setTickDensity(75);
    c->yAxis()->setTickDensity(30);

    //
    // In this demo, the time range can be from many hours to a few seconds. We can need to define
    // the date/time format the various cases.
    //

    // If all ticks are hour algined, we use "hh:nn<*br*>mmm dd" in bold font as the first label of
    // the Day, and "hh:nn" for other labels.
    c->xAxis()->setFormatCondition("align", 3600);
    c->xAxis()->setMultiFormat(Chart::StartOfDayFilter(), "<*font=bold*>{value|hh:nn<*br*>mmm dd}",
        Chart::AllPassFilter(), "{value|hh:nn}");

    // If all ticks are minute algined, then we use "hh:nn" as the label format.
    c->xAxis()->setFormatCondition("align", 60);
    c->xAxis()->setLabelFormat("{value|hh:nn}");

    // If all other cases, we use "hh:nn:ss" as the label format.
    c->xAxis()->setFormatCondition("else");
    c->xAxis()->setLabelFormat("{value|hh:nn:ss}");

    // We make sure the tick increment must be at least 1 second.
    c->xAxis()->setMinTickInc(1);

    //================================================================================
    // Output the chart
    //================================================================================

    // We need to update the track line too. If the mouse is moving on the chart (eg. if
    // the user drags the mouse on the chart to scroll it), the track line will be updated
    // in the MouseMovePlotArea event. Otherwise, we need to update the track line here.
    if (!viewer->isInMouseMoveEvent())
    {
        trackLineLabel(c, (0 == viewer->getChart()) ? c->getPlotArea()->getRightX() :
            viewer->getPlotAreaMouseX());
    }

    // Set the chart image to the QChartViewer
    delete viewer->getChart();
    viewer->setChart(c);
}

//
// Draw track cursor when mouse is moving over plotarea
//
void QtChartWindow::onMouseMovePlotArea(QMouseEvent *)
{
    trackLineLabel((XYChart *)m_ChartViewer->getChart(), m_ChartViewer->getPlotAreaMouseX());
    m_ChartViewer->updateDisplay();
}

//
// Draw the track line with data point labels
//
void QtChartWindow::trackLineLabel(XYChart *c, int mouseX)
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

