#include "pixelwidget.h"
#include <chthon/files.h>
#include <SDL2/SDL.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <streambuf>

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

PixelWidget::PixelWidget(const std::string & imageFileName, int width, int height)
	: renderer(0), quit(false), zoomFactor(4), color(0), fileName(imageFileName), canvas(32, 32), mode(DRAWING_MODE), wholeScreenChanged(true), do_draw_grid(false),
	dot_h(0), dot_v(0)
{
	if(Chthon::file_exists(fileName) && (width == 0 || height == 0)) {
		std::ifstream file(fileName.c_str());
		if(file) {
			std::string data((std::istreambuf_iterator<char>(file)),
					std::istreambuf_iterator<char>());
			try {
				canvas = Chthon::Pixmap(data);
			} catch(const Chthon::Pixmap::Exception & e) {
				std::cerr << e.what << std::endl;
				exit(1);
			}
		}
	} else {
		if(width != 0 && height != 0) {
			canvas = Chthon::Pixmap(width, height);
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
	std::ofstream file(fileName.c_str());
	if(file) {
		file << canvas.save();
	}
}

void PixelWidget::keyPressEvent(SDL_KeyboardEvent * event)
{
	if(mode == COLOR_INPUT_MODE) {
		bool isText = false;
		switch(event->keysym.sym) {
			case SDLK_BACKSPACE:
				if(colorEntered.size() > 1) {
					colorEntered.erase(colorEntered.size() - 1, 1);
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
			} else if(std::string("0123456789ABCDEFabcdef").find(c) != std::string::npos) {
				if(colorEntered == "-") {
					colorEntered = "";
				}
				colorEntered += c;
			}
		}
		update();
		return;
	}

	Chthon::Point shift;
	switch(event->keysym.sym) {
		case SDLK_k: case SDLK_UP: shift = Chthon::Point(0, -1); break;
		case SDLK_j: case SDLK_DOWN: shift = Chthon::Point(0, 1); break;
		case SDLK_h: case SDLK_LEFT: shift = Chthon::Point(-1, 0); break;
		case SDLK_l: case SDLK_RIGHT: shift = Chthon::Point(1, 0); break;
		case SDLK_y: shift = Chthon::Point(-1, -1); break;
		case SDLK_u: shift = Chthon::Point(1, -1); break;
		case SDLK_b: shift = Chthon::Point(-1, 1); break;
		case SDLK_n: shift = Chthon::Point(1, 1); break;

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
	if(!shift.null()) {
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
	selection.w += 1;
	selection.h += 1;
	std::vector<int> pixels(selection.w * selection.h);;
	for(int x = 0; x < selection.w; ++x) {
		for(int y = 0; y < selection.h; ++y) {
			pixels[x + y * selection.w] = canvas.pixel(selection.x + x, selection.y + y);
		}
	}
	for(int x = 0; x < selection.w; ++x) {
		for(int y = 0; y < selection.h; ++y) {
			int sx = x + cursor.x;
			int sy = y + cursor.y;
			canvas.set_pixel(sx, sy, pixels[x + y * selection.w]);
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
	selection.x = std::min(cursor.x, selection_start.x);
	selection.y = std::min(cursor.y, selection_start.y);
	selection.w = std::abs(cursor.x - selection_start.x);
	selection.h = std::abs(cursor.y - selection_start.y);
	cursor = Chthon::Point(selection.x, selection.y);
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
	canvas.floodfill(cursor.x, cursor.y, color);
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
	if(colorEntered.substr(0, 1) == "#") {
		colorEntered.erase(0, 1);
	}
	Chthon::Pixmap::Color value;
	if(colorEntered == "-") {
		value = Chthon::Pixmap::Color();
	} else if(colorEntered.size() % 2 == 0) {
		if(colorEntered.length() == 6) {
			int red = std::stol(colorEntered.substr(0, 2), nullptr, 16);
			int green = std::stol(colorEntered.substr(2, 2), nullptr, 16);
			int blue = std::stol(colorEntered.substr(4, 2), nullptr, 16);
			value = Chthon::Pixmap::Color(red, green, blue);
		}
	}
	canvas.set_color(color, value);
	wholeScreenChanged = true;
	update();
}

void PixelWidget::putColorAtCursor()
{
	canvas.set_pixel(cursor.x, cursor.y, color);
	wholeScreenChanged = false;
	update();
}

void PixelWidget::takeColorUnderCursor()
{
	color = indexAtPos(cursor);
	wholeScreenChanged = false;
	update();
}

void PixelWidget::shiftCanvas(const Chthon::Point & shift, int speed)
{
	canvasShift += shift * speed;
	update();
}

void PixelWidget::shiftCursor(const Chthon::Point & shift, int speed)
{
	if(speed < 1) {
		return;
	}
	int new_x = cursor.x + shift.x * speed;
	int new_y = cursor.y + shift.y * speed;
	if(mode == PASTE_MODE) {
		new_x = std::max(0, std::min(new_x, int(canvas.width()) - selection.w - 1));
		new_y = std::max(0, std::min(new_y, int(canvas.height()) - selection.h - 1));
	} else {
		new_x = std::max(0, std::min(new_x, int(canvas.width()) - 1));
		new_y = std::max(0, std::min(new_y, int(canvas.height()) - 1));
	}
	oldCursor = cursor;
	cursor = Chthon::Point(new_x, new_y);
	wholeScreenChanged = false;
	update();
}

void PixelWidget::centerCanvas()
{
	canvasShift = Chthon::Point();
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

uint PixelWidget::indexAtPos(const Chthon::Point & pos)
{
	return canvas.pixel(pos.x, pos.y);
}

Chthon::Pixmap::Color PixelWidget::indexToRealColor(uint index)
{
	return canvas.color(index);
}

void draw_line(SDL_Renderer * renderer, const Chthon::Point & a, const Chthon::Point & b)
{
	SDL_RenderDrawLine(renderer, a.x, a.y, b.x, b.y);
}

void PixelWidget::drawCursor(const SDL_Rect & cursor_rect)
{
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	Chthon::Point width = Chthon::Point(cursor_rect.w, 0);
	Chthon::Point height = Chthon::Point(0, cursor_rect.h);
	if(mode == PASTE_MODE) {
		SDL_Rect selection_rect;
		selection_rect.x = cursor_rect.x;
		selection_rect.y = cursor_rect.y;
		selection_rect.w = (selection.w + 1) * zoomFactor;
		selection_rect.h = (selection.h + 1) * zoomFactor;
		Chthon::Point topLeft(selection_rect.x, selection_rect.y);
		Chthon::Point bottomLeft(selection_rect.x, selection_rect.y + selection_rect.h);
		Chthon::Point topRight(selection_rect.x + selection_rect.w, selection_rect.y);
		Chthon::Point bottomRight(selection_rect.x + selection_rect.w, selection_rect.y + selection_rect.h);
		draw_line(renderer, topLeft - width, topRight + width);
		draw_line(renderer, bottomLeft - width, bottomRight + width);
		draw_line(renderer, topLeft - height, bottomLeft + height);
		draw_line(renderer, topRight - height, bottomRight + height);
	} else {
		Chthon::Point topLeft(cursor_rect.x, cursor_rect.y);
		Chthon::Point bottomLeft(cursor_rect.x, cursor_rect.y + cursor_rect.h);
		Chthon::Point topRight(cursor_rect.x + cursor_rect.w, cursor_rect.y);
		Chthon::Point bottomRight(cursor_rect.x + cursor_rect.w, cursor_rect.y + cursor_rect.h);
		draw_line(renderer, topLeft - width, topRight + width);
		draw_line(renderer, bottomLeft - width, bottomRight + width);
		draw_line(renderer, topLeft - height, bottomLeft + height);
		draw_line(renderer, topRight - height, bottomRight + height);
	}
}

void PixelWidget::drawGrid(const Chthon::Point & topLeft)
{
	SDL_Rect r;
	r.x = 0;
	r.y = 0;
	r.w = zoomFactor;
	r.h = zoomFactor;
	for(unsigned x = 0; x < canvas.width(); ++x) {
		for(unsigned y = 0; y < canvas.height(); ++y) {
			r.x = topLeft.x + x * zoomFactor;
			r.y = topLeft.y + y * zoomFactor;

			r.w = zoomFactor;
			r.h = 1;
			SDL_RenderCopy(renderer, dot_h, 0, &r);

			r.w = 1;
			r.h = zoomFactor;
			SDL_RenderCopy(renderer, dot_v, 0, &r);
		}
	}
}

std::string colorToString(const Chthon::Pixmap::Color & color)
{
	if(color.transparent) {
		return "None";
	}
	std::ostringstream out;
	out << '#';
	out.width(2);
	out.fill('0');
	out << std::hex;
	out << color.r << color.g << color.b;
	return out.str();
}

void PixelWidget::draw_pixel(const Chthon::Point & topLeft, const Chthon::Point & pos)
{
	Chthon::Pixmap::Color color = canvas.color(canvas.pixel(pos.x, pos.y));
	Chthon::Point p = topLeft + pos * zoomFactor;
	if(color.transparent) {
		if(zoomFactor % 2 != 0) {
			SDL_Rect r;
			r.x = p.x;
			r.y = p.y;
			r.w = zoomFactor;
			r.h = zoomFactor;
			int i = (pos.x + pos.y) % 2 == 0 ? 0 : 32;
			SDL_SetRenderDrawColor(renderer, i, i, i, 255);
			SDL_RenderFillRect(renderer, &r);
		} else {
			SDL_Rect r;
			r.x = p.x;
			r.y = p.y;
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
		SDL_Rect r;
		r.x = p.x;
		r.y = p.y;
		r.w = zoomFactor;
		r.h = zoomFactor;
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
		SDL_RenderFillRect(renderer, &r);
	}
}

SDL_Rect make_rect(const Chthon::Point & p, int width, int height)
{
	SDL_Rect result;
	result.x = p.x;
	result.y = p.y;
	result.w = width;
	result.h = height;
	return result;
}

void PixelWidget::update()
{
	Chthon::Point canvas_center = Chthon::Point(canvas.width() / 2, canvas.height() / 2);
	Chthon::Point rect_center = Chthon::Point(rect.w / 2, rect.h / 2);
	Chthon::Point leftTop = rect_center - (canvas_center - canvasShift) * zoomFactor;
	SDL_Rect imageRect = make_rect(leftTop, canvas.width() * zoomFactor, canvas.height() * zoomFactor);
	SDL_Rect cursorRect = make_rect(leftTop + cursor * zoomFactor, zoomFactor, zoomFactor);
	SDL_Rect oldCursorRect = make_rect(leftTop + oldCursor * zoomFactor, zoomFactor, zoomFactor);
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
		imageRect_adjusted.x = imageRect.x - 1;
		imageRect_adjusted.y = imageRect.y - 1;
		imageRect_adjusted.w = imageRect.w + 3;
		imageRect_adjusted.h = imageRect.h + 3;
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderDrawRect(renderer, &imageRect_adjusted);
		for(unsigned x = 0; x < canvas.width(); ++x) {
			for(unsigned y = 0; y < canvas.height(); ++y) {
				draw_pixel(leftTop, Chthon::Point(x, y));
			}
		}
	} else {
		drawCursor(oldCursorRect);
	}
	wholeScreenChanged = true;

	// Erase old selection.
	if(mode == COPY_MODE) {
		SDL_Rect old_selected_pixels;
		old_selected_pixels.x = std::min(oldCursor.x, selection_start.x);
		old_selected_pixels.y = std::min(oldCursor.y, selection_start.y);
		old_selected_pixels.w = std::abs(oldCursor.x - selection_start.x) + 1;
		old_selected_pixels.h = std::abs(oldCursor.y - selection_start.y) + 1;
		for(int x = old_selected_pixels.x; x <= old_selected_pixels.x + old_selected_pixels.w; ++x) {
			int y = old_selected_pixels.y;
			draw_pixel(leftTop, Chthon::Point(x, y));
			y = old_selected_pixels.y + old_selected_pixels.h;
			draw_pixel(leftTop, Chthon::Point(x, y));
		}
		for(int y = old_selected_pixels.y; y <= old_selected_pixels.y + old_selected_pixels.h; ++y) {
			int x = old_selected_pixels.x;
			draw_pixel(leftTop, Chthon::Point(x, y));
			x = old_selected_pixels.x + old_selected_pixels.w;
			draw_pixel(leftTop, Chthon::Point(x, y));
		}
	}

	if(do_draw_grid) {
		drawGrid(leftTop);
	}

	if(mode == COPY_MODE || mode == PASTE_MODE) {
		SDL_Rect selected_pixels;
		if(mode == PASTE_MODE) {
			selected_pixels = selection;
		} else {
			selected_pixels.x = std::min(oldCursor.x, selection_start.x);
			selected_pixels.y = std::min(oldCursor.y, selection_start.y);
			selected_pixels.w = std::abs(oldCursor.x - selection_start.x) + 1;
			selected_pixels.h = std::abs(oldCursor.y - selection_start.y) + 1;
		}
		SDL_Rect selection_rect = make_rect(
				leftTop + Chthon::Point(selected_pixels.x, selected_pixels.y) * zoomFactor,
				selected_pixels.w * zoomFactor, selected_pixels.h * zoomFactor
				);
		if(selection_rect.w > 0 && selection_rect.h > 0) {
			selection_rect.w -= 1;
			selection_rect.h -= 1;
		}

		SDL_Rect r;
		r.x = 0;
		r.y = 0;
		r.w = zoomFactor;
		r.h = 1;
		for(int x = selected_pixels.x; x <= selected_pixels.x + selected_pixels.w; ++x) {
			r.x = leftTop.x + x * zoomFactor - 1;
			r.y = leftTop.y + selected_pixels.y * zoomFactor - 1;
			SDL_RenderCopy(renderer, dot_h, 0, &r);

			r.y = leftTop.y + (selected_pixels.y + selected_pixels.h) * zoomFactor + zoomFactor - 1;
			SDL_RenderCopy(renderer, dot_h, 0, &r);
		}
		r.w = 1;
		r.h = zoomFactor;
		for(int y = selected_pixels.y; y <= selected_pixels.y + selected_pixels.h; ++y) {
			r.y = leftTop.y + y * zoomFactor - 1;
			r.x = leftTop.x + selected_pixels.x * zoomFactor - 1;
			SDL_RenderCopy(renderer, dot_v, 0, &r);

			r.x = leftTop.x + (selected_pixels.y + selected_pixels.h) * zoomFactor + zoomFactor - 1;
			SDL_RenderCopy(renderer, dot_v, 0, &r);
		}
	}

	draw_pixel(leftTop, cursor);
	drawCursor(cursorRect);

	Chthon::Point currentColorAreaShift;
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

	std::string line;
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

	for(const char & ch : line) {
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
