#ifndef KDateTimeSeriesChartView_H
#define KDateTimeSeriesChartView_H

#include <QChartView>
#include <QLineSeries>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QString>
#include "view_model/kdatetimeserieschartmodel.h"

/// <summary>
/// Time-series chart
/// </summary>
class KDateTimeSeriesChartView : public QtCharts::QChartView
{
    friend class KDateTimeSeriesChartModel;

public:
    KDateTimeSeriesChartView(
        KDateTimeSeriesChartModel* viewModel,
        const QString& chartTitle,
        const QString& axisXTitle,
        const QString& axisYTitle);

private:
    KDateTimeSeriesChartModel* m_viewModel;
    QString m_chartTitle;
    QString m_axisXTitle;
    QString m_axisYTitle;

    QtCharts::QLineSeries* m_series;
    QtCharts::QChart* m_chart;
    QtCharts::QDateTimeAxis* m_axisX;
    QtCharts::QValueAxis* m_axisY;

    void recalcRange();
    QString getAxisYTitle() const;

public slots:
    void onModelUpdated();
};

#endif // KDateTimeSeriesChartView_H
