#include <QtDebug>
#include <QtGui/QPainter>
#include "pixelwidget.h"

PixelWidget::PixelWidget(QWidget * parent)
	: QWidget(parent), zoomFactor(1)
{
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QSize PixelWidget::sizeHint() const
{
	return image.size() * zoomFactor;
}

QSize PixelWidget::minimumSizeHint() const
{
	return sizeHint();
}

void PixelWidget::load(const QString & fileName)
{
	image.load(fileName);
	resize(sizeHint());
	updateGeometry();
	update();
}

void PixelWidget::save(const QString & fileName)
{
	image.save(fileName);
}

void PixelWidget::zoomIn()
{
	++zoomFactor;
	resize(sizeHint());
	updateGeometry();
	update();
}

void PixelWidget::zoomOut()
{
	--zoomFactor;
	if(zoomFactor < 1) {
		zoomFactor = 1;
	}
	resize(sizeHint());
	updateGeometry();
	update();
}

void PixelWidget::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.drawImage(rect(), image);
}

