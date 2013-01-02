#include <QtDebug>
#include <QtGui/QPainter>
#include "pixelwidget.h"

PixelWidget::PixelWidget(QWidget * parent)
	: QWidget(parent)
{
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QSize PixelWidget::sizeHint() const
{
	return image.size();
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

void PixelWidget::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.drawImage(0, 0, image);
}

