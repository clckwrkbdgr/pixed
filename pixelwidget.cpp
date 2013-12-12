#include <QtDebug>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtGui/QPainter>
#include <QtGui/QKeyEvent>
#include "pixelwidget.h"

const int MIN_ZOOM_FACTOR = 2;

QString toString(const QList<int> & list)
{
	QStringList result;
	foreach(const int value, list) {
		result << QString::number(value);
	}
	if(result.isEmpty()) {
		return "None";
	}
	return result.join(", ");
}

PixelWidget::PixelWidget(const QString & imageFileName, const QSize & newSize, QWidget * parent)
	: QWidget(parent), zoomFactor(4), color(0), fileName(imageFileName), colorInputMode(false), wholeScreenChanged(true), do_draw_grid(false)
{
	setAttribute(Qt::WA_NoSystemBackground, true);
	if(QFile::exists(fileName) && newSize.isNull()) {
		canvas.load(fileName);

		QList<int> supportedDepths = QList<int>() << 1 << 8 << 32;
		if(!supportedDepths.contains(canvas.depth())) {
			QTextStream out(stdout);
			out << "Unsupported pixel depth: " << canvas.depth() << endl;
			out << "Supported depths limited to: " << toString(supportedDepths) << endl;
			exit(1);
		}

		if(canvas.colorCount() > 1 && canvas.colorCount() < 16) {
			canvas.setColorCount(16);
		} else if(canvas.colorCount() > 16 && canvas.colorCount() < 256) {
			canvas.setColorCount(256);
		} else {
			color = qRgba(0, 0, 0, 255);
		}
	} else {
		color = qRgba(0, 0, 0, 255);
		canvas = QImage(newSize.isNull() ? QSize(32, 32) : newSize, QImage::Format_ARGB32);
		canvas.fill(color);
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
		bool isText = false;
		switch(event->key()) {
			case Qt::Key_Backspace:
				if(colorEntered.size() > 1) {
					colorEntered.remove(colorEntered.size() - 1, 1);
				}
				break;
			case Qt::Key_Return: case Qt::Key_Enter: endColorInput(); break;
			case Qt::Key_Escape: colorEntered = ""; endColorInput(); break;
			default: isText = true;
		}
		if(isText) {
			foreach(const QChar & c, event->text()) {
				if(QString("0123456789ABCDEFabcdef").contains(c)) {
					colorEntered += c;
				}
			}
		}
		wholeScreenChanged = false;
		update();
		return;
	}

	QPoint shift;
	switch(event->key()) {
		case Qt::Key_K: case Qt::Key_Up: shift = QPoint(0, -1); break;
		case Qt::Key_J: case Qt::Key_Down: shift = QPoint(0, 1); break;
		case Qt::Key_H: case Qt::Key_Left: shift = QPoint(-1, 0); break;
		case Qt::Key_L: case Qt::Key_Right: shift = QPoint(1, 0); break;
		case Qt::Key_Y: shift = QPoint(-1, -1); break;
		case Qt::Key_U: shift = QPoint(1, -1); break;
		case Qt::Key_B: shift = QPoint(-1, 1); break;
		case Qt::Key_N: shift = QPoint(1, 1); break;

		case Qt::Key_PageUp: pickPrevColor(); break;
		case Qt::Key_PageDown: pickNextColor(); break;
		case Qt::Key_NumberSign: startColorInput(); break;
		case Qt::Key_Period: takeColorUnderCursor(); break;
		case Qt::Key_D: case Qt::Key_I: case Qt::Key_Space: putColorAtCursor(); break;
		case Qt::Key_P: floodFill(); break;
		case Qt::Key_Q: close(); break;
		case Qt::Key_S: if(event->modifiers().testFlag(Qt::ShiftModifier)) { canvas.save(fileName); }; break;
		case Qt::Key_G: if(event->modifiers().testFlag(Qt::ControlModifier)) { switch_draw_grid(); }; break;
		case Qt::Key_Equal: case Qt::Key_Plus: zoomIn(); break;
		case Qt::Key_Minus: zoomOut(); break;
		case Qt::Key_Home: centerCanvas(); break;
		default: QWidget::keyPressEvent(event);
	}
	oldCursor = cursor;
	if(!shift.isNull()) {
		if(event->modifiers().testFlag(Qt::ShiftModifier)) {
			shiftCanvas(shift);
		} else if(event->modifiers().testFlag(Qt::ControlModifier)) {
			shiftCursor(shift, 10);
		} else {
			shiftCursor(shift);
		}
	}
}

void PixelWidget::switch_draw_grid()
{
	do_draw_grid = !do_draw_grid;
	wholeScreenChanged = true;
	update();
}

uint indexAtPos(const QImage & image, const QPoint & pos)
{
	switch(image.depth()) {
		case 1: case 8: return image.pixelIndex(pos);
		case 32: return image.pixel(pos);
	}
	return 0;
}

void runFloodFill(QImage * image, const QPoint & pos, uint colorToPaintOver, uint colorWhichIsNeeded, int counter = 200000)
{
	if(counter < 0)
		return;
	if(image == NULL)
		return;
	if(!image->valid(pos))
		return;
	if(indexAtPos(*image, pos) == colorWhichIsNeeded)
		return;
	if(indexAtPos(*image, pos) != colorToPaintOver)
		return;
	image->setPixel(pos, colorWhichIsNeeded);
	runFloodFill(image, pos + QPoint( 1, 0), colorToPaintOver, colorWhichIsNeeded, counter - 1);
	runFloodFill(image, pos + QPoint(-1, 0), colorToPaintOver, colorWhichIsNeeded, counter - 1);
	runFloodFill(image, pos + QPoint(0,  1), colorToPaintOver, colorWhichIsNeeded, counter - 1);
	runFloodFill(image, pos + QPoint(0, -1), colorToPaintOver, colorWhichIsNeeded, counter - 1);
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
	wholeScreenChanged = false;
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
	wholeScreenChanged = false;
	update();
}

void PixelWidget::startColorInput()
{
	colorInputMode = true;
	colorEntered = "#";
	wholeScreenChanged = false;
	update();
}

void PixelWidget::endColorInput()
{
	colorInputMode = false;
	bool hasPalette = (canvas.colorCount() > 0);
	if(colorEntered.startsWith('#')) {
		colorEntered.remove(0, 1);
	}
	QColor value;
	if(colorEntered.size() % 2 == 0) {
		if(colorEntered.length() < 8) {
			colorEntered.prepend("ff");
		}
		if(colorEntered.length() == 8) {
			bool ok = false;
			int alpha = colorEntered.mid(0, 2).toInt(&ok, 16);
			int red = colorEntered.mid(2, 2).toInt(&ok, 16);
			int green = colorEntered.mid(4, 2).toInt(&ok, 16);
			int blue = colorEntered.mid(6, 2).toInt(&ok, 16);
			value = QColor(red, green, blue, alpha);
		}
	}
	if(value.isValid()) {
		if(hasPalette) {
			canvas.setColor(color, value.rgb());
		} else {
			color = value.rgba();
		}
	}
	wholeScreenChanged = false;
	update();
}

void PixelWidget::putColorAtCursor()
{
	canvas.setPixel(cursor, color);
	wholeScreenChanged = false;
	update();
}

void PixelWidget::takeColorUnderCursor()
{
	color = indexAtPos(cursor);
	wholeScreenChanged = false;
	update();
}

void PixelWidget::shiftCanvas(const QPoint & shift)
{
	canvasShift += shift;
	update();
}

void PixelWidget::shiftCursor(const QPoint & shift, int speed)
{
	if(speed < 1) {
		return;
	}
	QPoint newCursor = cursor + shift * speed;
	if(speed > 1) {
		newCursor.setX(qBound(0, newCursor.x(), canvas.width() - 1));
		newCursor.setY(qBound(0, newCursor.y(), canvas.height() - 1));
	}
	if(newCursor.x() < 0 || newCursor.x() >= canvas.width() || newCursor.y() < 0 || newCursor.y() >= canvas.height()) {
		return;
	}
	oldCursor = cursor;
	cursor = newCursor;
	wholeScreenChanged = false;
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
		case 32: return QColor::fromRgba(index);
	}
	return QColor();
}

void drawCursor(QPainter * painter, const QRect & rect)
{
	painter->setCompositionMode(QPainter::RasterOp_SourceXorDestination);
	painter->setPen(Qt::white);

	QPoint width = QPoint(rect.width(), 0);
	QPoint height = QPoint(0, rect.height());
	QVector<QPoint> lines;
	lines << rect.topLeft() - width << rect.topRight() + width;
	lines << rect.bottomLeft() - width << rect.bottomRight() + width;
	lines << rect.topLeft() - height << rect.bottomLeft() + height;
	lines << rect.topRight() - height << rect.bottomRight() + height;
	painter->drawLines(lines);

	painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
}

void drawGrid(QPainter * painter, const QSize & imageSize, const QPoint & topLeft, int zoomFactor)
{
	QVector<QPoint> white_lines, black_lines;
	for(int x = 1; x < imageSize.width(); ++x) {
		white_lines << topLeft + QPoint(x * zoomFactor, 0);
		white_lines << topLeft + QPoint(x * zoomFactor, imageSize.height() * zoomFactor);
		black_lines << topLeft + QPoint(0, 1) + QPoint(x * zoomFactor, 0);
		black_lines << topLeft + QPoint(0, 1) + QPoint(x * zoomFactor, imageSize.height() * zoomFactor);
	}
	for(int y = 1; y < imageSize.height(); ++y) {
		white_lines << topLeft + QPoint(0, y * zoomFactor);
		white_lines << topLeft + QPoint(imageSize.width() * zoomFactor, y * zoomFactor);
		black_lines << topLeft + QPoint(1, 0) + QPoint(0, y * zoomFactor);
		black_lines << topLeft + QPoint(1, 0) + QPoint(imageSize.width() * zoomFactor, y * zoomFactor);
	}

	QPen pen(Qt::DotLine);

	pen.setColor(Qt::white);
	painter->setPen(pen);
	painter->drawLines(white_lines);

	pen.setColor(Qt::black);
	painter->setPen(pen);
	painter->drawLines(black_lines);
}

QString colorToString(const QColor & color)
{
	if(color.alpha() < 255) {
		return QString("#%1%2%3%4").
			arg(color.alpha(), 2, 16, QChar('0')).
			arg(color.red(), 2, 16, QChar('0')).
			arg(color.green(), 2, 16, QChar('0')).
			arg(color.blue(), 2, 16, QChar('0'))
			;
	} else {
		return QString("#%1%2%3").
			arg(color.red(), 2, 16, QChar('0')).
			arg(color.green(), 2, 16, QChar('0')).
			arg(color.blue(), 2, 16, QChar('0'))
			;
	}
}

void PixelWidget::paintEvent(QPaintEvent*)
{
	QPoint leftTop = rect().center() - (canvas.rect().center() - canvasShift) * zoomFactor;
	QRect imageRect = QRect(leftTop, canvas.size() * zoomFactor);
	QRect cursorRect = QRect(leftTop + cursor * zoomFactor, QSize(zoomFactor, zoomFactor));
	QRect oldCursorRect = QRect(leftTop + oldCursor * zoomFactor, QSize(zoomFactor, zoomFactor));
	QRect currentColorRect = QRect(0, 0, 32, 32);
	QRect colorUnderCursorRect = QRect(0, 0, 8, 8).translated(24, 24);
	bool hasPalette = (canvas.colorCount() > 0);

	QPainter painter(this);
	painter.setBrush(Qt::NoBrush);

	if(wholeScreenChanged) {
		painter.fillRect(rect(), Qt::black);
		painter.drawRect(imageRect.adjusted(-1, -1, 0, 0));
		painter.drawImage(imageRect, canvas);

	} else {
		drawCursor(&painter, oldCursorRect);
	}
	wholeScreenChanged = true;

	if(do_draw_grid) {
		drawGrid(&painter, canvas.size(), imageRect.topLeft(), zoomFactor);
	}

	painter.setCompositionMode(QPainter::CompositionMode_Destination);
	painter.fillRect(cursorRect, indexToRealColor(indexAtPos(cursor)));
	drawCursor(&painter, cursorRect);
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

	QPoint currentColorAreaShift;
	if(hasPalette) {
		for(int i = 0; i < canvas.colorCount(); ++i) {
			painter.fillRect(QRect(0, 32 * i, 32, 32), canvas.color(i));
		}
		painter.drawRect(0, 0, 32, 32 * canvas.colorCount());
		currentColorAreaShift = QPoint(0, color * 32);
	}
	painter.fillRect(currentColorRect.translated(currentColorAreaShift), Qt::black);
	painter.setBrush(indexToRealColor(color));
	painter.drawRect(currentColorRect.translated(currentColorAreaShift));

	painter.fillRect(colorUnderCursorRect.translated(currentColorAreaShift), Qt::black);
	painter.setBrush(indexToRealColor(indexAtPos(cursor)));
	painter.drawRect(colorUnderCursorRect.translated(currentColorAreaShift));

	painter.fillRect(QRect(33, 0, width() - 33, 32), Qt::black);
	if(colorInputMode) {
		painter.drawText(currentColorAreaShift + QPoint(32, 32) + QPoint(5, -5), colorEntered);
	} else {
		painter.drawText(
				currentColorAreaShift + QPoint(32, 32) + QPoint(5, -5),
				colorToString(indexToRealColor(color)) + " [" + colorToString(indexToRealColor(indexAtPos(cursor))) + "]"
				);
	}
}

