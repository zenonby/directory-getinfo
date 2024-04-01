#include <cassert>
#include "defs.h"
#include <kdatetimeserieschartview.h>
#include "kdatetimeserieschartmodel.h"

KDateTimeSeriesChartModel::KDateTimeSeriesChartModel()
: m_divisor(FileSizeDivisor::Bytes)
{
}

FileSizeDivisor
KDateTimeSeriesChartModel::getFileSizeDivisor() const
{
	return m_divisor;
}

void
KDateTimeSeriesChartModel::setFileSizeDivisor(FileSizeDivisor divisor)
{
	m_divisor = divisor;

	emit modelUpdated();
}

const
QPointF&
KDateTimeSeriesChartModel::at(int index) const
{
	qreal divisorValue = FileSizeDivisorUtils::getDivisorValue(m_divisor);

	QPointF p = TBase::at(index);
	p.setY(round(FILE_SIZE_ROUNDING_FACTOR * p.y() / divisorValue) / FILE_SIZE_ROUNDING_FACTOR);

	return p;
}

void
KDateTimeSeriesChartModel::beginAppendSeriesDateValues()
{
	clear();
}

void
KDateTimeSeriesChartModel::appendSeriesDateValue(const QDateTime& dateTime, qreal y)
{
	qint64 msecs = dateTime.toMSecsSinceEpoch();
	append(msecs, y);
}

void
KDateTimeSeriesChartModel::endAppendSeriesDateValues()
{
	emit modelUpdated();
}
