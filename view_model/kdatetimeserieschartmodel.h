#ifndef KDateTimeSeriesChartModel_H
#define KDateTimeSeriesChartModel_H

#include <QLineSeries>
#include <QDateTime>
#include "FileSizeDivisor.h"

class KDateTimeSeriesChartView;

class KDateTimeSeriesChartModel : public QtCharts::QLineSeries
{
    Q_OBJECT

    typedef QtCharts::QLineSeries TBase;

public:
    KDateTimeSeriesChartModel();

    FileSizeDivisor getFileSizeDivisor() const;
    void setFileSizeDivisor(FileSizeDivisor divisor);

    const QPointF& at(int index) const;

    /// Clears previous series
    void beginAppendSeriesDateValues();

    /// Appends date-time value
    void appendSeriesDateValue(const QDateTime& dateTime, qreal y);

    /// Recalculates chart ranges so that the all data would be shown properly
    void endAppendSeriesDateValues();

private:
    FileSizeDivisor m_divisor;

    QList<QPointF> points() const = delete;
    QVector<QPointF> pointsVector() const = delete;

signals:
    void modelUpdated();
};

#endif // KDateTimeSeriesChartModel_H
