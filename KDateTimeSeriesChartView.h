#ifndef KDateTimeSeriesChartView_H
#define KDateTimeSeriesChartView_H

#include <QChartView>
#include <QLineSeries>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QString>

/// <summary>
/// Time-series chart
/// </summary>
class KDateTimeSeriesChartView : public QtCharts::QChartView
{
public:
    KDateTimeSeriesChartView(
        const QString& chartTitle,
        const QString& axisXTitle,
        const QString& axisYTitle);

    /// Clears previous series
    void beginAppendSeriesDateValues();

    /// Appends date-time value
    void appendSeriesDateValue(const QDateTime& dateTime, qreal y);

    /// Recalculates chart ranges so that the all data would be shown properly
    void endAppendSeriesDateValues();

private:
    QString m_chartTitle;
    QString m_axisXTitle;
    QString m_axisYTitle;

    QtCharts::QLineSeries* m_series;
    QtCharts::QChart* m_chart;
    QtCharts::QDateTimeAxis* m_axisX;
    QtCharts::QValueAxis* m_axisY;

    void recalcRange();
};

#endif // KDateTimeSeriesChartView_H
