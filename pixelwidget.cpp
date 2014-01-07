#include <QtDebug>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>
#include <QtGui/QPainter>
#include <QtGui/QKeyEvent>
#include "pixelwidget.h"

const int MIN_ZOOM_FACTOR = 2;

PixelWidget::PixelWidget(const QString & imageFileName, const QSize & newSize, QWidget * parent)
	: QWidget(parent), zoomFactor(4), color(0), fileName(imageFileName), canvas(32, 32), colorInputMode(false), wholeScreenChanged(true), do_draw_grid(false)
{
	setAttribute(Qt::WA_NoSystemBackground, true);
	if(QFile::exists(fileName) && newSize.isNull()) {
		QFile file(fileName);
		QString data;
		if(file.open(QFile::ReadOnly)) {
			QTextStream in(&file);
			data = in.readAll();
			try {
				canvas = Pixmap(data.toStdString());
			} catch(const Pixmap::Exception & e) {
				QTextStream err(stderr);
				err << QString::fromStdString(e.what) << endl;
				exit(1);
			}
		}
	} else {
		if(!newSize.isNull()) {
			canvas = Pixmap(newSize.width(), newSize.height());
		}
	}
	color = 0;
	update();
}

PixelWidget::~PixelWidget()
{
	save();
}

void PixelWidget::save()
{
	QFile file(fileName);
	if(file.open(QFile::WriteOnly)) {
		QString data = QString::fromStdString(canvas.save());
		QTextStream out(&file);
		out << data;
	}
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
				if(c == '-') {
					colorEntered = "-";
				} else if(QString("0123456789ABCDEFabcdef").contains(c)) {
					if(colorEntered == "-") {
						colorEntered = "";
					}
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
		case Qt::Key_S: if(event->modifiers().testFlag(Qt::ShiftModifier)) { save(); }; break;
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

void PixelWidget::floodFill()
{
	canvas.floodfill(cursor.x(), cursor.y(), color);
	update();
}

void PixelWidget::pickNextColor()
{
	unsigned newColor = color + 1;
	if(newColor >= canvas.color_count()) {
		return;
	}
	color = newColor;
	wholeScreenChanged = false;
	update();
}

void PixelWidget::pickPrevColor()
{
	if(color == 0) {
		return;
	}
	--color;
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
	if(colorEntered.startsWith('#')) {
		colorEntered.remove(0, 1);
	}
	Pixmap::Color value;
	if(colorEntered == "-") {
		value = Pixmap::Color();
	} else if(colorEntered.size() % 2 == 0) {
		if(colorEntered.length() == 6) {
			bool ok = false;
			int red = colorEntered.mid(2, 2).toInt(&ok, 16);
			int green = colorEntered.mid(4, 2).toInt(&ok, 16);
			int blue = colorEntered.mid(6, 2).toInt(&ok, 16);
			value = Pixmap::Color(red, green, blue);
		}
	}
	canvas.set_color(color, value);
	wholeScreenChanged = false;
	update();
}

void PixelWidget::putColorAtCursor()
{
	canvas.set_pixel(cursor.x(), cursor.y(), color);
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
	unsigned new_x = cursor.x() + shift.x() * speed;
	unsigned new_y = cursor.y() + shift.y() * speed;
	if(speed > 1) {
		new_x = qBound(0u, new_x, canvas.width() - 1);
		new_y = qBound(0u, new_y, canvas.height() - 1);
	}
	if(new_x >= canvas.width() || new_y >= canvas.height()) {
		return;
	}
	oldCursor = cursor;
	cursor = QPoint(new_x, new_y);
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
	return canvas.pixel(pos.x(), pos.y());
}

Pixmap::Color PixelWidget::indexToRealColor(uint index)
{
	return canvas.color(index);
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

QString colorToString(const Pixmap::Color & color)
{
	if(color.transparent) {
		return "None";
	}
	return QString("#%1%2%3").
		arg(color.r, 2, 16, QChar('0')).
		arg(color.g, 2, 16, QChar('0')).
		arg(color.b, 2, 16, QChar('0'))
		;
}

void PixelWidget::paintEvent(QPaintEvent*)
{
	QPoint canvas_center = QPoint(canvas.width() / 2, canvas.height() / 2);
	QSize canvas_size = QSize(canvas.width(), canvas.height());
	QPoint leftTop = rect().center() - (canvas_center - canvasShift) * zoomFactor;
	QRect imageRect = QRect(leftTop, canvas_size * zoomFactor);
	QRect cursorRect = QRect(leftTop + cursor * zoomFactor, QSize(zoomFactor, zoomFactor));
	QRect oldCursorRect = QRect(leftTop + oldCursor * zoomFactor, QSize(zoomFactor, zoomFactor));
	QRect currentColorRect = QRect(0, 0, 32, 32);
	QRect colorUnderCursorRect = QRect(0, 0, 8, 8).translated(24, 24);

	QPainter painter(this);
	painter.setBrush(Qt::NoBrush);

	if(wholeScreenChanged) {
		painter.fillRect(rect(), Qt::black);
		painter.drawRect(imageRect.adjusted(-1, -1, 0, 0));
		for(unsigned x = 0; x < canvas.width(); ++x) {
			for(unsigned y = 0; y < canvas.height(); ++y) {
				painter.fillRect(
						QRect(imageRect.topLeft() + QPoint(x * zoomFactor, y * zoomFactor), QSize(zoomFactor, zoomFactor)),
						canvas.color(canvas.pixel(x, y)).argb()
						);
			}
		}
	} else {
		drawCursor(&painter, oldCursorRect);
	}
	wholeScreenChanged = true;

	if(do_draw_grid) {
		drawGrid(&painter, QSize(canvas.width(), canvas.height()), imageRect.topLeft(), zoomFactor);
	}

	painter.setCompositionMode(QPainter::CompositionMode_Destination);
	painter.fillRect(cursorRect, indexToRealColor(indexAtPos(cursor)).argb());
	drawCursor(&painter, cursorRect);
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

	QPoint currentColorAreaShift;
	for(unsigned i = 0; i < canvas.color_count(); ++i) {
		painter.fillRect(QRect(0, 32 * i, 32, 32), canvas.color(i).argb());
	}
	painter.drawRect(0, 0, 32, 32 * canvas.color_count());
	currentColorAreaShift = QPoint(0, color * 32);
	painter.fillRect(currentColorRect.translated(currentColorAreaShift), Qt::black);
	painter.setBrush(QColor(indexToRealColor(color).argb()));
	painter.drawRect(currentColorRect.translated(currentColorAreaShift));

	painter.fillRect(colorUnderCursorRect.translated(currentColorAreaShift), Qt::black);
	painter.setBrush(QColor(indexToRealColor(indexAtPos(cursor)).argb()));
	painter.drawRect(colorUnderCursorRect.translated(currentColorAreaShift));

	painter.fillRect(QRect(33, 0, width() - 33, 32), Qt::black);
	if(colorInputMode) {
		painter.drawText(QPoint(32, 32) + QPoint(5, -5), colorEntered);
	} else {
		painter.drawText(
				QPoint(32, 32) + QPoint(5, -5),
				colorToString(indexToRealColor(color)) + " [" + colorToString(indexToRealColor(indexAtPos(cursor))) + "]"
				);
	}
}

