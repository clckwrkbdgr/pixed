#include <QtDebug>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>
#include <QtGui/QPainter>
#include <QtGui/QKeyEvent>
#include "pixelwidget.h"

const int MIN_ZOOM_FACTOR = 2;

enum { DRAWING_MODE, COLOR_INPUT_MODE, COPY_MODE, PASTE_MODE };

PixelWidget::PixelWidget(const QString & imageFileName, const QSize & newSize, QWidget * parent)
	: QWidget(parent), zoomFactor(4), color(0), fileName(imageFileName), canvas(32, 32), mode(DRAWING_MODE), wholeScreenChanged(true), do_draw_grid(false)
{
	setAttribute(Qt::WA_NoSystemBackground, true);
	if(QFile::exists(fileName) && newSize.isNull()) {
		QFile file(fileName);
		QString data;
		if(file.open(QFile::ReadOnly)) {
			QTextStream in(&file);
			data = in.readAll();
			try {
				canvas = Chthon::Pixmap(data.toStdString());
			} catch(const Chthon::Pixmap::Exception & e) {
				QTextStream err(stderr);
				err << QString::fromStdString(e.what) << endl;
				exit(1);
			}
		}
	} else {
		if(!newSize.isNull()) {
			canvas = Chthon::Pixmap(newSize.width(), newSize.height());
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
	if(mode == COLOR_INPUT_MODE) {
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

		case Qt::Key_Q: close(); break;
		case Qt::Key_S: if(event->modifiers().testFlag(Qt::ShiftModifier)) { save(); }; break;
		case Qt::Key_G: if(event->modifiers().testFlag(Qt::ControlModifier)) { switch_draw_grid(); }; break;
		case Qt::Key_Equal: case Qt::Key_Plus: zoomIn(); break;
		case Qt::Key_Minus: zoomOut(); break;
		case Qt::Key_Home: centerCanvas(); break;
		default: QWidget::keyPressEvent(event);
	}
	if(mode == COPY_MODE) {
		switch(event->key()) {
			case Qt::Key_Escape: mode = DRAWING_MODE; break;
			case Qt::Key_V: startPasteMode(); break;
			default: break;
		}
	} else if(mode == PASTE_MODE) {
		switch(event->key()) {
			case Qt::Key_Return: pasteSelection(); break;
			case Qt::Key_Escape: mode = DRAWING_MODE; break;
			default: break;
		}
	} else if(mode == DRAWING_MODE) {
		switch(event->key()) {
			case Qt::Key_C: startCopyMode(); break;
			case Qt::Key_A: color = canvas.add_color(Chthon::Pixmap::Color()); startColorInput(); break;
			case Qt::Key_PageUp: pickPrevColor(); break;
			case Qt::Key_PageDown: pickNextColor(); break;
			case Qt::Key_NumberSign: startColorInput(); break;
			case Qt::Key_Period: takeColorUnderCursor(); break;
			case Qt::Key_D: case Qt::Key_I: case Qt::Key_Space: putColorAtCursor(); break;
			case Qt::Key_P: floodFill(); break;
		}
	}
	oldCursor = cursor;
	if(!shift.isNull()) {
		int speed = 1;
		if(event->modifiers().testFlag(Qt::ControlModifier)) {
			speed = 10;
		}
		if(event->modifiers().testFlag(Qt::ShiftModifier)) {
			shiftCanvas(shift, speed);
		} else {
			shiftCursor(shift, speed);
		}
	}
	update();
}

void PixelWidget::pasteSelection()
{
	selection.setSize(selection.size() + QSize(1, 1));
	QVector<int> pixels(selection.width() * selection.height());;
	for(int x = 0; x < selection.width(); ++x) {
		for(int y = 0; y < selection.height(); ++y) {
			pixels[x + y * selection.width()] = canvas.pixel(selection.left() + x, selection.top() + y);
		}
	}
	for(int x = 0; x < selection.width(); ++x) {
		for(int y = 0; y < selection.height(); ++y) {
			int sx = x + cursor.x();
			int sy = y + cursor.y();
			canvas.set_pixel(sx, sy, pixels[x + y * selection.width()]);
		}
	}
	mode = DRAWING_MODE;
	update();
}

void PixelWidget::startCopyMode()
{
	mode = COPY_MODE;
	selection_start = cursor;
	update();
}

void PixelWidget::startPasteMode()
{
	mode = PASTE_MODE;
	selection = QRect(
			QPoint(
				qMin(cursor.x(), selection_start.x()),
				qMin(cursor.y(), selection_start.y())
				),
			QSize(
				qAbs(cursor.x() - selection_start.x()),
				qAbs(cursor.y() - selection_start.y())
				)
			);
	cursor = selection.topLeft();
	update();
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
	mode = COLOR_INPUT_MODE;
	colorEntered = "#";
	wholeScreenChanged = false;
	update();
}

void PixelWidget::endColorInput()
{
	mode = DRAWING_MODE;
	if(colorEntered.startsWith('#')) {
		colorEntered.remove(0, 1);
	}
	Chthon::Pixmap::Color value;
	if(colorEntered == "-") {
		value = Chthon::Pixmap::Color();
	} else if(colorEntered.size() % 2 == 0) {
		if(colorEntered.length() == 6) {
			bool ok = false;
			int red = colorEntered.mid(0, 2).toInt(&ok, 16);
			int green = colorEntered.mid(2, 2).toInt(&ok, 16);
			int blue = colorEntered.mid(4, 2).toInt(&ok, 16);
			value = Chthon::Pixmap::Color(red, green, blue);
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

void PixelWidget::shiftCanvas(const QPoint & shift, int speed)
{
	canvasShift += shift * speed;
	update();
}

void PixelWidget::shiftCursor(const QPoint & shift, int speed)
{
	if(speed < 1) {
		return;
	}
	int new_x = cursor.x() + shift.x() * speed;
	int new_y = cursor.y() + shift.y() * speed;
	if(mode == PASTE_MODE) {
		new_x = qBound(0, new_x, int(canvas.width()) - selection.width() - 1);
		new_y = qBound(0, new_y, int(canvas.height()) - selection.height() - 1);
	} else {
		new_x = qBound(0, new_x, int(canvas.width()) - 1);
		new_y = qBound(0, new_y, int(canvas.height()) - 1);
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

Chthon::Pixmap::Color PixelWidget::indexToRealColor(uint index)
{
	return canvas.color(index);
}

void PixelWidget::drawCursor(QPainter * painter, const QRect & rect)
{
	painter->setCompositionMode(QPainter::RasterOp_SourceXorDestination);
	painter->setPen(Qt::white);

	QPoint width = QPoint(rect.width(), 0);
	QPoint height = QPoint(0, rect.height());
	QVector<QPoint> lines;
	if(mode == PASTE_MODE) {
		QRect selection_rect = QRect(rect.topLeft(), (selection.size() + QSize(1, 1)) * zoomFactor);
		lines << selection_rect.topLeft() - width << selection_rect.topRight() + width;
		lines << selection_rect.bottomLeft() - width << selection_rect.bottomRight() + width;
		lines << selection_rect.topLeft() - height << selection_rect.bottomLeft() + height;
		lines << selection_rect.topRight() - height << selection_rect.bottomRight() + height;
	} else {
		lines << rect.topLeft() - width << rect.topRight() + width;
		lines << rect.bottomLeft() - width << rect.bottomRight() + width;
		lines << rect.topLeft() - height << rect.bottomLeft() + height;
		lines << rect.topRight() - height << rect.bottomRight() + height;
	}
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

QString colorToString(const Chthon::Pixmap::Color & color)
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

void PixelWidget::draw_pixel(QPainter * painter, const QPoint & topLeft, const QPoint & pos)
{
	Chthon::Pixmap::Color color = canvas.color(canvas.pixel(pos.x(), pos.y()));
	if(color.transparent) {
		painter->fillRect(QRect(topLeft + pos * zoomFactor, QSize(zoomFactor, zoomFactor)), Qt::magenta);
	} else {
		painter->fillRect(QRect(topLeft + pos * zoomFactor, QSize(zoomFactor, zoomFactor)), QColor(color.argb()));
	}
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
				draw_pixel(&painter, leftTop, QPoint(x, y));
			}
		}
	} else {
		drawCursor(&painter, oldCursorRect);
	}
	wholeScreenChanged = true;

	// Erase old selection.
	if(mode == COPY_MODE) {
		QRect old_selected_pixels = QRect(
				QPoint(
					qMin(oldCursor.x(), selection_start.x()),
					qMin(oldCursor.y(), selection_start.y())
					),
				QSize(
					qAbs(oldCursor.x() - selection_start.x()),
					qAbs(oldCursor.y() - selection_start.y())
					)
				);
		old_selected_pixels.setWidth(old_selected_pixels.width() + 1);
		old_selected_pixels.setHeight(old_selected_pixels.height() + 1);
		for(int x = old_selected_pixels.left(); x <= old_selected_pixels.right(); ++x) {
			int y = old_selected_pixels.top();
			draw_pixel(&painter, leftTop, QPoint(x, y));
			y = old_selected_pixels.bottom();
			draw_pixel(&painter, leftTop, QPoint(x, y));
		}
		for(int y = old_selected_pixels.top(); y <= old_selected_pixels.bottom(); ++y) {
			int x = old_selected_pixels.left();
			draw_pixel(&painter, leftTop, QPoint(x, y));
			x = old_selected_pixels.right();
			draw_pixel(&painter, leftTop, QPoint(x, y));
		}
	}

	if(do_draw_grid) {
		drawGrid(&painter, QSize(canvas.width(), canvas.height()), imageRect.topLeft(), zoomFactor);
	}

	if(mode == COPY_MODE || mode == PASTE_MODE) {
		QRect selected_pixels = (mode == PASTE_MODE) ? selection : QRect(
				QPoint(
					qMin(cursor.x(), selection_start.x()),
					qMin(cursor.y(), selection_start.y())
					),
				QSize(
					qAbs(cursor.x() - selection_start.x()),
					qAbs(cursor.y() - selection_start.y())
					)
				);
		selected_pixels.setWidth(selected_pixels.width() + 1);
		selected_pixels.setHeight(selected_pixels.height() + 1);
		QRect selection_rect = QRect(
				leftTop + selected_pixels.topLeft() * zoomFactor,
				selected_pixels.size() * zoomFactor
				);
		if(selection_rect.isValid()) {
			selection_rect.setSize(selection_rect.size() - QSize(1, 1));
		}
		QPen solid_pen(Qt::SolidLine);
		solid_pen.setColor(Qt::white);
		painter.setPen(solid_pen);
		painter.drawRect(selection_rect);

		QPen dot_pen(Qt::DotLine);
		dot_pen.setColor(Qt::black);
		painter.setPen(dot_pen);
		painter.drawRect(selection_rect);
	}

	painter.setCompositionMode(QPainter::CompositionMode_Destination);
	draw_pixel(&painter, leftTop, cursor);
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
	switch(mode) {
		case COLOR_INPUT_MODE:
		painter.drawText(QPoint(32, 32) + QPoint(5, -5), colorEntered);
		break;

		case DRAWING_MODE:
		painter.drawText(
				QPoint(32, 32) + QPoint(5, -5),
				colorToString(indexToRealColor(color)) + " [" + colorToString(indexToRealColor(indexAtPos(cursor))) + "]"
				);
		break;
	}
}

