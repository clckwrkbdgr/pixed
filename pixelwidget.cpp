#include <QtDebug>
#include <QtCore/QFile>
#include <QtGui/QPainter>
#include <QtGui/QKeyEvent>
#include "pixelwidget.h"

const int MIN_ZOOM_FACTOR = 2;

PixelWidget::PixelWidget(const QString & imageFileName, QWidget * parent)
	: QWidget(parent), zoomFactor(4), fileName(imageFileName)
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

void PixelWidget::keyPressEvent(QKeyEvent * event)
{
	QPoint shift;
	switch(event->key()) {
		case Qt::Key_Up: shift = QPoint(0, -1); break;
		case Qt::Key_Down: shift = QPoint(0, 1); break;
		case Qt::Key_Left: shift = QPoint(-1, 0); break;
		case Qt::Key_Right: shift = QPoint(1, 0); break;

		case Qt::Key_Q: close(); break;
		case Qt::Key_Plus: zoomIn(); break;
		case Qt::Key_Minus: zoomOut(); break;
		case Qt::Key_Home: centerCanvas(); break;
		default: QWidget::keyPressEvent(event);
	}
	if(!shift.isNull()) {
		if(event->modifiers().testFlag(Qt::ShiftModifier)) {
			shiftCanvas(shift);
		}
	}
}

void PixelWidget::shiftCanvas(const QPoint & shift)
{
	canvasShift += shift;
	update();
}

void PixelWidget::centerCanvas()
{
	canvasShift = QPoint();
	update();
}

void PixelWidget::zoomIn()
{
	zoomFactor++;
	update();
}

void PixelWidget::zoomOut()
{
	zoomFactor--;
	if(zoomFactor < MIN_ZOOM_FACTOR) {
		zoomFactor = MIN_ZOOM_FACTOR;
	}
	update();
}

void PixelWidget::paintEvent(QPaintEvent*)
{
	QPoint leftTop = rect().center() - (canvas.rect().center() - canvasShift) * zoomFactor;
	QRect imageRect = QRect(leftTop, canvas.size() * zoomFactor);

	QPainter painter(this);
	painter.fillRect(rect(), Qt::black);
	painter.setPen(Qt::red);
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(imageRect.adjusted(-1, -1, 0, 0));
	painter.drawImage(imageRect, canvas);
}

