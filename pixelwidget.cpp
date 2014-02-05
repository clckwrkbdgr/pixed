#include <QtDebug>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>
#include "pixelwidget.h"
#include <SDL2/SDL.h>

SDL_Texture * create_dotted_texture(SDL_Renderer * renderer, int size, bool is_h)
{
	SDL_Texture * result = 0;
	SDL_Surface * surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
			is_h ? size : 1, is_h ? 1 : size, 32,
			0x00ff0000,
			0x0000ff00,
			0x000000ff,
			0xff000000
			);
	if(SDL_MUSTLOCK(surface)) {
		SDL_LockSurface(surface);
	}
	SDL_Rect r;
	r.x = 0;
	r.y = 0;
	r.w = is_h ? size : 1;
	r.h = is_h ? 1 : size;
	SDL_FillRect(surface, &r, 0);
	for(int x = 0; x < size; ++x) {
		Uint32 * pixel = (Uint32*)surface->pixels;
		pixel += x;
		*pixel = ((size + x) % 2 == 0) ? 0xff000000 : 0xffffffff;
	}
	if(SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
	result = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, r.w, r.h);
	SDL_UpdateTexture(result, 0, surface->pixels, surface->pitch);
	SDL_SetTextureBlendMode(result, SDL_BLENDMODE_BLEND);
	SDL_FreeSurface(surface);
	return result;
}



const int MIN_ZOOM_FACTOR = 2;

enum { DRAWING_MODE, COLOR_INPUT_MODE, COPY_MODE, PASTE_MODE };

PixelWidget::PixelWidget(const QString & imageFileName, const QSize & newSize)
	: renderer(0), quit(false), zoomFactor(4), color(0), fileName(imageFileName), canvas(32, 32), mode(DRAWING_MODE), wholeScreenChanged(true), do_draw_grid(false),
	dot_h(0), dot_v(0)
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
		case SDLK_EQUALS: case SDLK_KP_PLUS:  case SDLK_PLUS: zoomIn(); break;
		case SDLK_KP_MINUS: case SDLK_MINUS: zoomOut(); break;
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
			case SDLK_3: if(event->keysym.mod & (KMOD_RSHIFT | KMOD_LSHIFT)) { startColorInput(); } break;
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

void PixelWidget::recreate_dot_textures()
{
	if(dot_h) {
		SDL_DestroyTexture(dot_h);
	}
	dot_h = create_dotted_texture(renderer, zoomFactor, true);

	if(dot_v) {
		SDL_DestroyTexture(dot_v);
	}
	dot_v = create_dotted_texture(renderer, zoomFactor, false);
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
	recreate_dot_textures();
	update();
}

void PixelWidget::zoomOut()
{
	zoomFactor--;
	if(zoomFactor < MIN_ZOOM_FACTOR) {
		zoomFactor = MIN_ZOOM_FACTOR;
	}
	recreate_dot_textures();
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

void draw_line(SDL_Renderer * renderer, const QPoint & a, const QPoint & b)
{
	SDL_RenderDrawLine(renderer, a.x(), a.y(), b.x(), b.y());
}

void PixelWidget::drawCursor(const QRect & cursor_rect)
{
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	QPoint width = QPoint(cursor_rect.width(), 0);
	QPoint height = QPoint(0, cursor_rect.height());
	QVector<SDL_Point> lines;
	if(mode == PASTE_MODE) {
		QRect selection_rect = QRect(cursor_rect.topLeft(), (selection.size() + QSize(1, 1)) * zoomFactor);
		draw_line(renderer, selection_rect.topLeft() - width, selection_rect.topRight() + width);
		draw_line(renderer, selection_rect.bottomLeft() - width, selection_rect.bottomRight() + width);
		draw_line(renderer, selection_rect.topLeft() - height, selection_rect.bottomLeft() + height);
		draw_line(renderer, selection_rect.topRight() - height, selection_rect.bottomRight() + height);
	} else {
		draw_line(renderer, cursor_rect.topLeft() - width, cursor_rect.topRight() + width);
		draw_line(renderer, cursor_rect.bottomLeft() - width, cursor_rect.bottomRight() + width);
		draw_line(renderer, cursor_rect.topLeft() - height, cursor_rect.bottomLeft() + height);
		draw_line(renderer, cursor_rect.topRight() - height, cursor_rect.bottomRight() + height);
	}
	SDL_RenderDrawLines(renderer, lines.constData(), lines.count());
}

void PixelWidget::drawGrid(const QPoint & topLeft)
{
	SDL_Rect r;
	r.x = 0;
	r.y = 0;
	r.w = zoomFactor;
	r.h = zoomFactor;
	for(unsigned x = 0; x < canvas.width(); ++x) {
		for(unsigned y = 0; y < canvas.height(); ++y) {
			r.x = topLeft.x() + x * zoomFactor;
			r.y = topLeft.y() + y * zoomFactor;

			r.w = zoomFactor;
			r.h = 1;
			SDL_RenderCopy(renderer, dot_h, 0, &r);

			r.w = 1;
			r.h = zoomFactor;
			SDL_RenderCopy(renderer, dot_v, 0, &r);
		}
	}
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
			r.w = zoomFactor / 2;
			r.h = zoomFactor / 2;

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

	if(wholeScreenChanged) {
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		SDL_Rect imageRect_adjusted;
		imageRect_adjusted.x = imageRect.x() - 1;
		imageRect_adjusted.y = imageRect.y() - 1;
		imageRect_adjusted.w = imageRect.width() + 3;
		imageRect_adjusted.h = imageRect.height() + 3;
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
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

		SDL_Rect r;
		r.x = 0;
		r.y = 0;
		r.w = zoomFactor;
		r.h = 1;
		for(int x = selected_pixels.left(); x <= selected_pixels.right(); ++x) {
			r.x = leftTop.x() + x * zoomFactor - 1;
			r.y = leftTop.y() + selected_pixels.top() * zoomFactor - 1;
			SDL_RenderCopy(renderer, dot_h, 0, &r);

			r.y = leftTop.y() + selected_pixels.bottom() * zoomFactor + zoomFactor - 1;
			SDL_RenderCopy(renderer, dot_h, 0, &r);
		}
		r.w = 1;
		r.h = zoomFactor;
		for(int y = selected_pixels.top(); y <= selected_pixels.bottom(); ++y) {
			r.y = leftTop.y() + y * zoomFactor - 1;
			r.x = leftTop.x() + selected_pixels.left() * zoomFactor - 1;
			SDL_RenderCopy(renderer, dot_v, 0, &r);

			r.x = leftTop.x() + selected_pixels.right() * zoomFactor + zoomFactor - 1;
			SDL_RenderCopy(renderer, dot_v, 0, &r);
		}
	}

	draw_pixel(leftTop, cursor);
	drawCursor(cursorRect);

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
	palette_rect.h = 32 * canvas.color_count();
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

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_Rect text_rect;
	text_rect.x = 33;
	text_rect.y = 0;
	text_rect.w = rect.w - 33;
	text_rect.h = 32;
	SDL_RenderFillRect(renderer, &text_rect);

	QString line;
	switch(mode) {
		case COLOR_INPUT_MODE:
			line = colorEntered;
			break;
		case DRAWING_MODE:
			line = colorToString(indexToRealColor(color)) + " [" + colorToString(indexToRealColor(indexAtPos(cursor))) + "]";
			break;
	}

	SDL_Rect dest_rect;
	dest_rect.x = 33;
	dest_rect.y = 2;
	dest_rect.w = font.getCharRect(0).w;
	dest_rect.h = font.getCharRect(0).h;

	for(const char & ch : line.toStdString()) {
		SDL_Rect char_rect = font.getCharRect(ch);
		SDL_RenderCopy(renderer, font.getFont(), &char_rect, &dest_rect);
		dest_rect.x += dest_rect.w;
	}
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

	font.init(renderer);
	recreate_dot_textures();

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
