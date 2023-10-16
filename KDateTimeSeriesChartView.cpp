#include <QtCharts>
#include "kdatetimeserieschartview.h"

KDateTimeSeriesChartView::KDateTimeSeriesChartView(
        const QString& chartTitle,
        const QString& axisXTitle,
        const QString& axisYTitle)
    : m_chartTitle(chartTitle),
      m_axisXTitle(axisXTitle),
      m_axisYTitle(axisYTitle),
      m_series(nullptr),
      m_axisX(nullptr),
      m_axisY(nullptr)
{
    m_series = new QLineSeries();

    QChart *m_chart = new QChart();
    m_chart->addSeries(m_series);
    m_chart->legend()->hide();
    m_chart->setTitle(m_chartTitle);
    m_chart->layout()->setContentsMargins(0, 0, 0, 0);
    m_chart->setBackgroundRoundness(0);

    m_axisX = new QDateTimeAxis;
    m_axisX->setFormat("yyyy.MM.dd");
    m_axisX->setTitleText(m_axisXTitle);
    m_axisX->setLabelsAngle(-45);
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_series->attachAxis(m_axisX);

    m_axisY = new QValueAxis;
    m_axisY->setTitleText(m_axisYTitle);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisY);

    setChart(m_chart);
    setRenderHint(QPainter::Antialiasing);
}

void
KDateTimeSeriesChartView::beginAppendSeriesDateValues()
{
    m_series->clear();
}

void
KDateTimeSeriesChartView::appendSeriesDateValue(const QDateTime& dateTime, qreal y)
{
    qint64 msecs = dateTime.toMSecsSinceEpoch();
    m_series->append(msecs, y);
}

void
KDateTimeSeriesChartView::endAppendSeriesDateValues()
{
    recalcRange();
}

void
KDateTimeSeriesChartView::recalcRange()
{
    qint64 minMSec = 0;
    qint64 maxMSec = 0;
    qreal minY = 0;
    qreal maxY = 0;

    int count = m_series->count();
    for (int i = 0; i < count; ++i)
    {
        auto p = m_series->at(i);
        qint64 msec = p.x();
        qreal y = p.y();

        if (0 == i)
        {
            minMSec = maxMSec = msec;
            minY = maxY = y;
        }
        else
        {
            minMSec = std::min(minMSec, msec);
            maxMSec = std::max(maxMSec, msec);

            minY = std::min(minY, y);
            maxY = std::max(maxY, y);
        }
    }

    if (0 < count)
    {
        auto dtMin = QDateTime(QDateTime::fromMSecsSinceEpoch(minMSec).date());
        auto dtMax = QDateTime(QDateTime::fromMSecsSinceEpoch(maxMSec).date().addDays(1));

        m_axisX->setRange(dtMin, dtMax);

        m_axisY->setRange(floor(minY) - 1, ceil(maxY) + 1);
    }
}
