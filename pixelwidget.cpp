#include <QtDebug>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>
#include "pixelwidget.h"
#include <SDL2/SDL.h>

const int MIN_ZOOM_FACTOR = 2;

enum { DRAWING_MODE, COLOR_INPUT_MODE, COPY_MODE, PASTE_MODE };

PixelWidget::PixelWidget(const QString & imageFileName, const QSize & newSize)
	: renderer(0), quit(false), zoomFactor(4), color(0), fileName(imageFileName), canvas(32, 32), mode(DRAWING_MODE), wholeScreenChanged(true), do_draw_grid(false)
{
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

void PixelWidget::close()
{
	quit = true;
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

void PixelWidget::keyPressEvent(SDL_KeyboardEvent * event)
{
	if(mode == COLOR_INPUT_MODE) {
		bool isText = false;
		switch(event->keysym.sym) {
			case SDLK_BACKSPACE:
				if(colorEntered.size() > 1) {
					colorEntered.remove(colorEntered.size() - 1, 1);
				}
				break;
			case SDLK_RETURN: case SDLK_RETURN2: endColorInput(); break;
			case SDLK_ESCAPE: colorEntered = ""; endColorInput(); break;
			default: isText = true;
		}
		if(isText) {
			std::string text = SDL_GetKeyName(event->keysym.sym);
			char c = text[0];
			if(c == '-') {
				colorEntered = "-";
			} else if(QString("0123456789ABCDEFabcdef").contains(c)) {
				if(colorEntered == "-") {
					colorEntered = "";
				}
				colorEntered += c;
			}
		}
		update();
		return;
	}

	QPoint shift;
	switch(event->keysym.sym) {
		case SDLK_k: case SDLK_UP: shift = QPoint(0, -1); break;
		case SDLK_j: case SDLK_DOWN: shift = QPoint(0, 1); break;
		case SDLK_h: case SDLK_LEFT: shift = QPoint(-1, 0); break;
		case SDLK_l: case SDLK_RIGHT: shift = QPoint(1, 0); break;
		case SDLK_y: shift = QPoint(-1, -1); break;
		case SDLK_u: shift = QPoint(1, -1); break;
		case SDLK_b: shift = QPoint(-1, 1); break;
		case SDLK_n: shift = QPoint(1, 1); break;

		case SDLK_q: close(); break;
		case SDLK_s: if(event->keysym.mod & (KMOD_RSHIFT | KMOD_LSHIFT)) { save(); }; break;
		case SDLK_g: if(event->keysym.mod & (KMOD_RCTRL | KMOD_LCTRL)) { switch_draw_grid(); }; break;
		case SDLK_EQUALS: case SDLK_PLUS: zoomIn(); break;
		case SDLK_MINUS: zoomOut(); break;
		case SDLK_HOME: centerCanvas(); break;
	}
	if(mode == COPY_MODE) {
		switch(event->keysym.sym) {
			case SDLK_ESCAPE: mode = DRAWING_MODE; break;
			case SDLK_v: startPasteMode(); break;
			default: break;
		}
	} else if(mode == PASTE_MODE) {
		switch(event->keysym.sym) {
			case SDLK_RETURN: case SDLK_RETURN2: pasteSelection(); break;
			case SDLK_ESCAPE: mode = DRAWING_MODE; break;
			default: break;
		}
	} else if(mode == DRAWING_MODE) {
		switch(event->keysym.sym) {
			case SDLK_c: startCopyMode(); break;
			case SDLK_a: color = canvas.add_color(Chthon::Pixmap::Color()); startColorInput(); break;
			case SDLK_PAGEUP: pickPrevColor(); break;
			case SDLK_PAGEDOWN: pickNextColor(); break;
			case SDLK_HASH: startColorInput(); break;
			case SDLK_PERIOD: takeColorUnderCursor(); break;
			case SDLK_d: case SDLK_i: case SDLK_SPACE: putColorAtCursor(); break;
			case SDLK_p: floodFill(); break;
		}
	}
	oldCursor = cursor;
	if(!shift.isNull()) {
		int speed = 1;
		if(event->keysym.mod & (KMOD_RCTRL | KMOD_LCTRL)) {
			speed = 10;
		}
		if(event->keysym.mod & (KMOD_RSHIFT | KMOD_LSHIFT)) {
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
	wholeScreenChanged = true;
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

SDL_Point to_sdl_point(const QPoint & point)
{
	SDL_Point result;
	result.x = point.x();
	result.y = point.y();
	return result;
}

void PixelWidget::drawCursor(const QRect & cursor_rect)
{
	// TODO painter->setCompositionMode(QPainter::RasterOp_SourceXorDestination);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	QPoint width = QPoint(cursor_rect.width(), 0);
	QPoint height = QPoint(0, cursor_rect.height());
	QVector<SDL_Point> lines;
	if(mode == PASTE_MODE) {
		QRect selection_rect = QRect(cursor_rect.topLeft(), (selection.size() + QSize(1, 1)) * zoomFactor);
		lines << to_sdl_point(selection_rect.topLeft() - width) << to_sdl_point(selection_rect.topRight() + width);
		lines << to_sdl_point(selection_rect.bottomLeft() - width) << to_sdl_point(selection_rect.bottomRight() + width);
		lines << to_sdl_point(selection_rect.topLeft() - height) << to_sdl_point(selection_rect.bottomLeft() + height);
		lines << to_sdl_point(selection_rect.topRight() - height) << to_sdl_point(selection_rect.bottomRight() + height);
	} else {
		lines << to_sdl_point(cursor_rect.topLeft() - width) << to_sdl_point(cursor_rect.topRight() + width);
		lines << to_sdl_point(cursor_rect.bottomLeft() - width) << to_sdl_point(cursor_rect.bottomRight() + width);
		lines << to_sdl_point(cursor_rect.topLeft() - height) << to_sdl_point(cursor_rect.bottomLeft() + height);
		lines << to_sdl_point(cursor_rect.topRight() - height) << to_sdl_point(cursor_rect.bottomRight() + height);
	}
	SDL_RenderDrawLines(renderer, lines.constData(), lines.count());

	// TODO painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
}

void PixelWidget::drawGrid(const QPoint & topLeft)
{
	QVector<SDL_Point> white_lines, black_lines;
	for(unsigned x = 1; x < canvas.width(); ++x) {
		white_lines << to_sdl_point(topLeft + QPoint(x * zoomFactor, 0));
		white_lines << to_sdl_point(topLeft + QPoint(x * zoomFactor, canvas.height() * zoomFactor));
		black_lines << to_sdl_point(topLeft + QPoint(0, 1) + QPoint(x * zoomFactor, 0));
		black_lines << to_sdl_point(topLeft + QPoint(0, 1) + QPoint(x * zoomFactor, canvas.height() * zoomFactor));
	}
	for(unsigned y = 1; y < canvas.height(); ++y) {
		white_lines << to_sdl_point(topLeft + QPoint(0, y * zoomFactor));
		white_lines << to_sdl_point(topLeft + QPoint(canvas.width() * zoomFactor, y * zoomFactor));
		black_lines << to_sdl_point(topLeft + QPoint(1, 0) + QPoint(0, y * zoomFactor));
		black_lines << to_sdl_point(topLeft + QPoint(1, 0) + QPoint(canvas.width() * zoomFactor, y * zoomFactor));
	}

	// TODO QPen pen(Qt::DotLine);

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	// TODO painter->setPen(pen);
	SDL_RenderDrawLines(renderer, white_lines.constData(), white_lines.count());

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	// TODO painter->setPen(pen);
	SDL_RenderDrawLines(renderer, black_lines.constData(), black_lines.count());
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

void PixelWidget::draw_pixel(const QPoint & topLeft, const QPoint & pos)
{
	QSize pixelSize = QSize(zoomFactor + 1, zoomFactor + 1);
	Chthon::Pixmap::Color color = canvas.color(canvas.pixel(pos.x(), pos.y()));
	if(color.transparent) {
		if(zoomFactor % 2 != 0) {
			QRect q_r = QRect(topLeft + pos * zoomFactor, pixelSize);
			SDL_Rect r;
			r.x = q_r.x();
			r.y = q_r.y();
			r.w = q_r.width();
			r.h = q_r.height();
			int i = (pos.x() + pos.y()) % 2 == 0 ? 0 : 32;
			SDL_SetRenderDrawColor(renderer, i, i, i, 255);
			SDL_RenderFillRect(renderer, &r);
		} else {
			QRect q_r = QRect(topLeft + pos * zoomFactor, pixelSize / 2);
			SDL_Rect r;
			r.x = q_r.x();
			r.y = q_r.y();
			r.w = q_r.width();
			r.h = q_r.height();

			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderFillRect(renderer, &r);
			r.x += zoomFactor / 2;
			r.y += zoomFactor / 2;
			SDL_RenderFillRect(renderer, &r);

			SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255);
			r.x -= zoomFactor / 2;
			SDL_RenderFillRect(renderer, &r);
			r.x += zoomFactor / 2;
			r.y -= zoomFactor / 2;
			SDL_RenderFillRect(renderer, &r);
		}
	} else {
		QRect q_r = QRect(topLeft + pos * zoomFactor, pixelSize);
		SDL_Rect r;
		r.x = q_r.x();
		r.y = q_r.y();
		r.w = q_r.width();
		r.h = q_r.height();
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
		SDL_RenderFillRect(renderer, &r);
	}
}

void PixelWidget::update()
{
	QPoint canvas_center = QPoint(canvas.width() / 2, canvas.height() / 2);
	QSize canvas_size = QSize(canvas.width(), canvas.height());
	QPoint rect_center = QPoint(rect.w / 2, rect.h / 2);
	QPoint leftTop = rect_center - (canvas_center - canvasShift) * zoomFactor;
	QRect imageRect = QRect(leftTop, canvas_size * zoomFactor);
	QRect cursorRect = QRect(leftTop + cursor * zoomFactor, QSize(zoomFactor, zoomFactor));
	QRect oldCursorRect = QRect(leftTop + oldCursor * zoomFactor, QSize(zoomFactor, zoomFactor));
	SDL_Rect colorUnderCursorRect;
	colorUnderCursorRect.x = 24;
	colorUnderCursorRect.y = 24;
	colorUnderCursorRect.w = 8;
	colorUnderCursorRect.h = 8;
	SDL_Rect currentColorRect;
	currentColorRect.x = 0;
	currentColorRect.y = 0;
	currentColorRect.w = 32;
	currentColorRect.h = 32;

	// TODO painter.setBrush(Qt::NoBrush);

	if(wholeScreenChanged) {
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		SDL_Rect imageRect_adjusted;
		imageRect_adjusted.x = imageRect.x() - 1;
		imageRect_adjusted.y = imageRect.y() - 1;
		imageRect_adjusted.w = imageRect.width();
		imageRect_adjusted.h = imageRect.height();
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderDrawRect(renderer, &imageRect_adjusted);
		for(unsigned x = 0; x < canvas.width(); ++x) {
			for(unsigned y = 0; y < canvas.height(); ++y) {
				draw_pixel(leftTop, QPoint(x, y));
			}
		}
	} else {
		drawCursor(oldCursorRect);
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
			draw_pixel(leftTop, QPoint(x, y));
			y = old_selected_pixels.bottom();
			draw_pixel(leftTop, QPoint(x, y));
		}
		for(int y = old_selected_pixels.top(); y <= old_selected_pixels.bottom(); ++y) {
			int x = old_selected_pixels.left();
			draw_pixel(leftTop, QPoint(x, y));
			x = old_selected_pixels.right();
			draw_pixel(leftTop, QPoint(x, y));
		}
	}

	if(do_draw_grid) {
		drawGrid(imageRect.topLeft());
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
		SDL_Rect r_selection_rect;
		r_selection_rect.x = selection_rect.x();
		r_selection_rect.y = selection_rect.y();
		r_selection_rect.w = selection_rect.width();
		r_selection_rect.h = selection_rect.height();

		// TODO QPen solid_pen(Qt::SolidLine);
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderDrawRect(renderer, &r_selection_rect);

		// TODO QPen dot_pen(Qt::DotLine);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderDrawRect(renderer, &r_selection_rect);
	}

	// TODO painter.setCompositionMode(QPainter::CompositionMode_Destination);
	draw_pixel(leftTop, cursor);
	drawCursor(cursorRect);
	// TODO painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

	QPoint currentColorAreaShift;
	SDL_Rect palette_rect;
	palette_rect.x = 0;
	palette_rect.y = 0;
	palette_rect.w = 32;
	palette_rect.h = 32;
	for(unsigned i = 0; i < canvas.color_count(); ++i) {
		palette_rect.y = i * 32;
		Chthon::Pixmap::Color color = canvas.color(i);
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
		SDL_RenderFillRect(renderer, &palette_rect);
	}
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	palette_rect.y = 0;
	palette_rect.w = 32 * canvas.color_count();
	SDL_RenderDrawRect(renderer, &palette_rect);

	currentColorRect.y += color * 32;
	Chthon::Pixmap::Color color_value = canvas.color(color);
	SDL_SetRenderDrawColor(renderer, color_value.r, color_value.g, color_value.b, 255);
	SDL_RenderFillRect(renderer, &currentColorRect);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDrawRect(renderer, &currentColorRect);

	colorUnderCursorRect.y += color * 32;
	color_value = canvas.color(indexAtPos(cursor));
	SDL_SetRenderDrawColor(renderer, color_value.r, color_value.g, color_value.b, 255);
	SDL_RenderFillRect(renderer, &colorUnderCursorRect);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDrawRect(renderer, &colorUnderCursorRect);

	/* TODO
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
	*/
}

int PixelWidget::exec()
{
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window * window = SDL_CreateWindow(
			"Pixed",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			0, 0,
			SDL_WINDOW_FULLSCREEN_DESKTOP
			);
	SDL_ShowCursor(0);
	rect.x = 0;
	rect.y = 0;
	SDL_GetWindowSize(window, &rect.w, &rect.h);

	renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	SDL_Event event;
	while(!quit) {
		while(SDL_PollEvent(&event)) {
			update();
			SDL_RenderPresent(renderer);

			if(event.type == SDL_KEYDOWN) {
				keyPressEvent(&event.key);
			} else if(event.type == SDL_QUIT) {
				quit = true;
			}
		}
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
