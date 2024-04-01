#include <cassert>
#include <QtCharts>
#include "kdatetimeserieschartview.h"

KDateTimeSeriesChartView::KDateTimeSeriesChartView(
        KDateTimeSeriesChartModel* viewModel,
        const QString& chartTitle,
        const QString& axisXTitle,
        const QString& axisYTitle)
    : m_viewModel(viewModel),
      m_chartTitle(chartTitle),
      m_axisXTitle(axisXTitle),
      m_axisYTitle(axisYTitle),
      m_series(nullptr),
      m_axisX(nullptr),
      m_axisY(nullptr)
{
    assert(!!m_viewModel);

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
    m_axisY->setTitleText(getAxisYTitle());
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisY);

    setChart(m_chart);
    setRenderHint(QPainter::Antialiasing);

    connect(m_viewModel, &KDateTimeSeriesChartModel::modelUpdated, this, &KDateTimeSeriesChartView::onModelUpdated);
}

QString
KDateTimeSeriesChartView::getAxisYTitle() const
{
    auto divisor = m_viewModel->getFileSizeDivisor();
    return m_axisYTitle + "," + FileSizeDivisorUtils::getDivisorSuffix(divisor);
}

void
KDateTimeSeriesChartView::recalcRange()
{
    qint64 minMSec = 0;
    qint64 maxMSec = 0;
    qreal minY = 0;
    qreal maxY = 0;

    const int count = m_series->count();
    const int modelCount = m_viewModel->count();

    bool isNewSeries = count != modelCount;

    if (isNewSeries)
        m_series->clear();

    for (int i = 0; i < modelCount; ++i)
    {
        auto p = m_viewModel->at(i);
        qint64 msec = p.x();
        qreal y = p.y();

        if (isNewSeries)
            m_series->append(p);
        else
            m_series->replace(i, p);

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

    if (0 < modelCount)
    {
        auto dtMin = QDateTime(QDateTime::fromMSecsSinceEpoch(minMSec).date());
        auto dtMax = QDateTime(QDateTime::fromMSecsSinceEpoch(maxMSec).date().addDays(1));

        m_axisX->setRange(dtMin, dtMax);

        m_axisY->setRange(floor(minY) - 1, ceil(maxY) + 1);
    }
}

void
KDateTimeSeriesChartView::onModelUpdated()
{
    m_axisY->setTitleText(getAxisYTitle());
    recalcRange();
}
