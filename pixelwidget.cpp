#include <QtDebug>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtGui/QPainter>
#include <QtGui/QKeyEvent>
#include "pixelwidget.h"

const int MIN_ZOOM_FACTOR = 2;

QString toString(const QList<int> & list)
{
	QString result = list.isEmpty() ? "None" : QString::number(list.first());
	foreach(const int value, list) {
		result += QString(", %1").arg(value);
	}
	return result;
}

PixelWidget::PixelWidget(const QString & imageFileName, QWidget * parent)
	: QWidget(parent), explicitCursor(false), zoomFactor(4), color(0), fileName(imageFileName), colorInputMode(false)
{
	if(QFile::exists(fileName)) {
		canvas.load(fileName);

		QList<int> supportedDepths = QList<int>() << 1 << 8 << 32;
		if(!supportedDepths.contains(canvas.depth())) {
			QTextStream out(stdout);
			out << "Unsupported pixel depth: " << canvas.depth();
			out << "Supported depths limited to: " << toString(supportedDepths);
			exit(1);
		}

		QList<int> supportedColorCounts = QList<int>() << 1 << 16 << 256 << 0;
		if(!supportedColorCounts.contains(canvas.colorCount())) {
			QTextStream out(stdout);
			out << "Unsupported color count: " << canvas.colorCount();
			out << "Supported counts limited to: " << toString(supportedColorCounts);
			exit(1);
		}
	} else {
		canvas = QImage(QSize(32, 32), QImage::Format_RGB32);
		canvas.fill(0);
	}
	update();
}

PixelWidget::~PixelWidget()
{
	canvas.save(fileName);
}

void PixelWidget::keyPressEvent(QKeyEvent * event)
{
	if(colorInputMode) {
		switch(event->key()) {
			case Qt::Key_Backspace: if(colorEntered.size() > 1) colorEntered.remove(colorEntered.size() - 1); break;
			case Qt::Key_Return: case Qt::Key_Enter: endColorInput(); break;
			case Qt::Key_Escape: colorEntered = ""; endColorInput(); break;
			default: colorEntered += event->text();
		}
		update();
	}

	QPoint shift;
	switch(event->key()) {
		case Qt::Key_Up: shift = QPoint(0, -1); break;
		case Qt::Key_Down: shift = QPoint(0, 1); break;
		case Qt::Key_Left: shift = QPoint(-1, 0); break;
		case Qt::Key_Right: shift = QPoint(1, 0); break;

		case Qt::Key_PageUp: pickPrevColor(); break;
		case Qt::Key_PageDown: pickNextColor(); break;
		case Qt::Key_NumberSign: startColorInput(); break;
		case Qt::Key_Period: takeColorUnderCursor(); break;
		case Qt::Key_I: case Qt::Key_Space: putColorAtCursor(); break;
		case Qt::Key_P: floodFill(); break;
		case Qt::Key_V: toggleExplicitCursor(); break;
		case Qt::Key_Q: close(); break;
		case Qt::Key_Plus: zoomIn(); break;
		case Qt::Key_Minus: zoomOut(); break;
		case Qt::Key_Home: centerCanvas(); break;
		default: QWidget::keyPressEvent(event);
	}
	if(!shift.isNull()) {
		if(event->modifiers().testFlag(Qt::ShiftModifier)) {
			shiftCanvas(shift);
		} else {
			shiftCursor(shift);
		}
	}
}

uint indexAtPos(const QImage & image, const QPoint & pos)
{
	switch(image.depth()) {
		case 1: case 8: return image.pixelIndex(pos);
		case 32: return image.pixel(pos);
	}
	return 0;
}

void runFloodFill(QImage * image, const QPoint & pos, uint colorToPaintOver, uint colorWhichIsNeeded)
{
	if(image == NULL)
		return;
	if(!image->valid(pos))
		return;
	if(indexAtPos(*image, pos) != colorToPaintOver)
		return;
	image->setPixel(pos, colorWhichIsNeeded);
	runFloodFill(image, pos + QPoint( 1, 0), colorToPaintOver, colorWhichIsNeeded);
	runFloodFill(image, pos + QPoint(-1, 0), colorToPaintOver, colorWhichIsNeeded);
	runFloodFill(image, pos + QPoint(0,  1), colorToPaintOver, colorWhichIsNeeded);
	runFloodFill(image, pos + QPoint(0, -1), colorToPaintOver, colorWhichIsNeeded);
}

void PixelWidget::floodFill()
{
	runFloodFill(&canvas, cursor, indexAtPos(cursor), color);
	update();
}

void PixelWidget::pickNextColor()
{
	bool hasPalette = (canvas.colorCount() > 0);
	if(!hasPalette)
		return;
	int newColor = color + 1;
	if(newColor >= canvas.colorCount())
		return;
	color = newColor;
	update();
}

void PixelWidget::pickPrevColor()
{
	bool hasPalette = (canvas.colorCount() > 0);
	if(!hasPalette)
		return;
	int newColor = color - 1;
	if(newColor < 0)
		return;
	color = newColor;
	update();
}

void PixelWidget::startColorInput()
{
	colorInputMode = true;
	colorEntered = "#";
	update();
}

void PixelWidget::endColorInput()
{
	colorInputMode = false;
	bool hasPalette = (canvas.colorCount() > 0);
	QColor value = QColor(colorEntered);
	if(value.isValid()) {
		if(hasPalette) {
			canvas.setColor(color, value.rgb());
		} else {
			color = value.rgb();
		}
	}
	update();
}

void PixelWidget::putColorAtCursor()
{
	canvas.setPixel(cursor, color);
	update();
}

void PixelWidget::takeColorUnderCursor()
{
	color = indexAtPos(cursor);
	update();
}

void PixelWidget::toggleExplicitCursor()
{
	explicitCursor = !explicitCursor;
	update();
}

void PixelWidget::shiftCanvas(const QPoint & shift)
{
	canvasShift += shift;
	update();
}

void PixelWidget::shiftCursor(const QPoint & shift)
{
	QPoint newCursor = cursor + shift;
	if(newCursor.x() < 0 || newCursor.x() >= canvas.width() || newCursor.y() < 0 || newCursor.y() >= canvas.height()) {
		return;
	}
	cursor = newCursor;
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

uint PixelWidget::indexAtPos(const QPoint & pos)
{
	switch(canvas.depth()) {
		case 1: case 8: return canvas.pixelIndex(pos);
		case 32: return canvas.pixel(pos);
	}
	return 0;
}

QColor PixelWidget::indexToRealColor(uint index)
{
	switch(canvas.depth()) {
		case 1: case 8: return canvas.color(index);
		case 32: return QColor(index);
	}
	return QColor();
}

void PixelWidget::paintEvent(QPaintEvent*)
{
	QPoint leftTop = rect().center() - (canvas.rect().center() - canvasShift) * zoomFactor;
	QRect imageRect = QRect(leftTop, canvas.size() * zoomFactor);
	QRect cursorRect = QRect(leftTop + cursor * zoomFactor, QSize(zoomFactor, zoomFactor));
	QRect currentColorRect = QRect(0, 0, 32, 32);
	QRect colorUnderCursorRect = QRect(0, 0, 8, 8).translated(24, 24);
	bool hasPalette = (canvas.colorCount() > 0);

	QPainter painter(this);
	painter.fillRect(rect(), Qt::black);
	painter.setPen(Qt::red);
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(imageRect.adjusted(-1, -1, 0, 0));
	painter.drawImage(imageRect, canvas);
	if(explicitCursor) {
		painter.drawLine(cursorRect.left(), 0, cursorRect.left(), height());
		painter.drawLine(cursorRect.right(), 0, cursorRect.right(), height());
		painter.drawLine(0, cursorRect.top(), width(), cursorRect.top());
		painter.drawLine(0, cursorRect.bottom(), width(), cursorRect.bottom());
	} else {
		painter.drawRect(cursorRect);
	}
	QPoint currentColorAreaShift;
	if(hasPalette) {
		for(int i = 0; i < canvas.colorCount(); ++i) {
			painter.fillRect(QRect(0, 32 * i, 32, 32), canvas.color(i));
		}
		painter.drawRect(0, 0, 32, 32 * canvas.colorCount());

		currentColorAreaShift = QPoint(32, color * 32);
	}
	painter.setBrush(indexToRealColor(color));
	painter.drawRect(currentColorRect.translated(currentColorAreaShift));
	painter.setBrush(indexToRealColor(indexAtPos(cursor)));
	painter.drawRect(colorUnderCursorRect.translated(currentColorAreaShift));
	if(colorInputMode) {
		painter.drawText(currentColorAreaShift + QPoint(32, 32), colorEntered);
	}
}

