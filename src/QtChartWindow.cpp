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
    m_ChartViewer->setMouseWheelZoomRatio(1.1);

    connect(m_ChartViewer, SIGNAL(viewPortChanged()), SLOT(onViewPortChanged()));
    connect(m_ChartViewer, SIGNAL(mouseMovePlotArea(QMouseEvent*)), SLOT(onMouseMovePlotArea(QMouseEvent*)));

    m_HScrollBar = new QScrollBar(Qt::Horizontal, m_ChartViewer);
    connect(m_HScrollBar, SIGNAL(valueChanged(int)), SLOT(onHScrollBarChanged(int)));

    ui->verticalLayout_2->setSpacing(4);
    ui->verticalLayout_2->setMargin(2);
    ui->verticalLayout_2->addWidget(m_ChartViewer);
    ui->verticalLayout_2->addWidget(m_HScrollBar);

    m_currentIndex = 0;

    m_nextDataTime = QDateTime::currentDateTime();
    QTimer *dataRateTimer = new QTimer(this);
    dataRateTimer->start(DataInterval);
    connect(dataRateTimer, SIGNAL(timeout()), SLOT(getData()));

    m_ChartUpdateTimer = new QTimer(this);
    connect(m_ChartUpdateTimer, SIGNAL(timeout()), SLOT(onChartUpdateTimer()));

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
}

QtChartWindow::~QtChartWindow()
{
}

void QtChartWindow::resizeEvent(QResizeEvent *event)
{
    widthPlot = ui->gbChartPlot->width() - 20;
    heightPlot = ui->gbChartPlot->height() - 70;
}

void QtChartWindow::addItem(const QString name)
{
    QLabel* label;
    QLabel* value;

    int labelRow = curvesWidgetLayout->rowCount();

    selectAllCheckBox = new QCheckBox("", this);
    connect(selectAllCheckBox, SIGNAL(clicked(bool)), this, SLOT(selectAllCurves(bool)));
    curvesWidgetLayout->addWidget(selectAllCheckBox, labelRow, 0, 1, 2);

    label = new QLabel(this);
    label->setText(name);
    curvesWidgetLayout->addWidget(label, labelRow, 2);

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

            i.value()->setText(str);
        }
    }
}

void QtChartWindow::onMouseUsageChanged(int mouseUsage)
{
    m_ChartViewer->setMouseUsage(mouseUsage);
}

void QtChartWindow::onSave(bool)
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save", "chartdirector_demo", "PNG (*.png);;JPG (*.jpg);;GIF (*.gif);;BMP (*.bmp);;SVG (*.svg);;PDF (*.pdf)");

    if (!fileName.isEmpty())
    {
        BaseChart *c = m_ChartViewer->getChart();

        if (0 != c)
        {
            c->makeChart(fileName.toUtf8().constData());
        }
    }
}

void QtChartWindow::onUpdatePeriodChanged(QString text)
{
    m_ChartUpdateTimer->start(text.toInt());
}

void QtChartWindow::onChartUpdateTimer()
{
    if (m_currentIndex > 0)
    {
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

void QtChartWindow::onViewPortChanged()
{
    // In addition to updating the chart, we may also need to update other controls that
    // changes based on the view port.
    updateControls(m_ChartViewer);

    // Update the chart if necessary
    if (m_ChartViewer->needUpdateChart())
        drawChart(m_ChartViewer);
}

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

    qDebug()<<"Index: "<<m_currentIndex;
}

void QtChartWindow::drawChart(QChartViewer *viewer)
{
    double viewPortStartDate = viewer->getValueAtViewPort("x", viewer->getViewPortLeft());
    double viewPortEndDate = viewer->getValueAtViewPort("x", viewer->getViewPortLeft() + viewer->getViewPortWidth());

    if (m_currentIndex > 0)
    {
        int startIndex = (int)floor(Chart::bSearch(DoubleArray(dataSeries[0].constData(), m_currentIndex), viewPortStartDate));
        int endIndex = (int)ceil(Chart::bSearch(DoubleArray(dataSeries[0].constData(), m_currentIndex), viewPortEndDate));
        int noOfPoints = endIndex - startIndex + 1;


        XYChart *c = new XYChart(widthPlot, heightPlot);

        c->setPlotArea(55, 50, c->getWidth() - 85, c->getHeight() - 80, c->linearGradientColor(0, 50, 0, c->getHeight() - 35, 0xf0f6ff, 0xa0c0ff), -1, Chart::Transparent, 0xffffff, 0xffffff);
        c->setClipping();
        c->addTitle("    Realtime Chart with Zoom/Scroll and Track Line", "arial.ttf", 18);

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

        layer->setXData(DoubleArray(dataSeries[0].constData() + startIndex, noOfPoints));
        layer->addDataSet(DoubleArray(dataSeries[1].constData() + startIndex, noOfPoints), 0xff0000, "Alpha");
        layer->addDataSet(DoubleArray(dataSeries[2].constData() + startIndex, noOfPoints), 0x00cc00, "Beta");
        layer->addDataSet(DoubleArray(dataSeries[3].constData() + startIndex, noOfPoints), 0xffcc00, "Gama");

        if (m_currentIndex > 0)
            c->xAxis()->setDateScale(viewPortStartDate, viewPortEndDate);

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
        }

        delete viewer->getChart();
        viewer->setChart(c);
    }
}

void QtChartWindow::onMouseMovePlotArea(QMouseEvent *)
{
    trackLineLabel((XYChart *)m_ChartViewer->getChart(), m_ChartViewer->getPlotAreaMouseX());
    m_ChartViewer->updateDisplay();
}

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
