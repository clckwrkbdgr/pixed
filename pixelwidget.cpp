#include <QtDebug>
#include <QtCore/QFile>
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include "pixelwidget.h"

PixelWidget::PixelWidget(const QString & imageFileName, QWidget * parent)
	: QWidget(parent), fileName(imageFileName)
{
	if(QFile::exists(fileName)) {
		canvas.load(fileName);
	} else {
		canvas = QImage(QSize(32, 32), QImage::Format_RGB32);
		canvas.fill(qRgb(255, 255, 255));
	}
	update();
}

PixelWidget::~PixelWidget()
{
	canvas.save(fileName);
}

void PixelWidget::paintEvent(QPaintEvent*)
{
	QRect imageRect = QRect(rect().center() - canvas.rect().center() * 4, canvas.size() * 4);

	QPainter painter(this);
	painter.fillRect(rect(), Qt::black);
	painter.setPen(Qt::red);
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(imageRect.adjusted(-1, -1, 0, 0));
	painter.drawImage(imageRect, canvas);
}

