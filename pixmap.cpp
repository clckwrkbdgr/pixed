#include "pixmap.h"
#include <QtDebug>

Pixmap::Color::Color()
	: transparent(true), r(0), g(0), b(0)
{
}

Pixmap::Color::Color(uint8_t c_r, uint8_t c_g, uint8_t c_b)
	: transparent(false), r(c_r), g(c_g), b(c_b)
{
}

uint32_t Pixmap::Color::argb() const
{
	if(transparent) {
		return 0;
	}
	return (0xff << 24) | (r << 16) | (g << 8) | b;
}

Pixmap::Color Pixmap::Color::from_argb(uint32_t color)
{
	if((color >> 24) == 0) {
		return Color();
	}
	return Color((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
}

bool Pixmap::Color::operator==(const Color & other) const
{
	return (transparent == other.transparent) || (r == other.r && g == other.g && b == other.b);
}

		
Pixmap::Pixmap(unsigned pixmap_width, unsigned pixmap_height, unsigned palette_size)
	: w(pixmap_width), h(pixmap_height), pixels(w * h, 0), palette(palette_size > 1 ? palette_size : 1, Color(0, 0, 0))
{
}

bool Pixmap::valid(unsigned x, unsigned y) const
{
	return x < w && y < h;
}

unsigned Pixmap::width() const
{
	return w;
}

unsigned Pixmap::height() const
{
	return h;
}

unsigned Pixmap::pixel(unsigned x, unsigned y) const
{
	if(valid(x, y)) {
		return pixels[x + y * w];
	}
	return 0;
}

unsigned Pixmap::color_count() const
{
	return palette.size();
}

Pixmap::Color Pixmap::color(unsigned index) const
{
	if(index < palette.size()) {
		return palette[index];
	}
	return Color();
}

bool Pixmap::resize(unsigned new_width, unsigned new_height)
{
	if(new_width < 1 || new_height < 1) {
		return false;
	}
	std::vector<unsigned> new_pixels(new_width * new_height, 0);
	std::vector<unsigned>::const_iterator src = pixels.begin();
	std::vector<unsigned>::iterator dest = new_pixels.begin();
	unsigned min_width = std::min(w, new_width);
	unsigned col = 0;
	unsigned rows = std::min(h, new_height);
	size_t src_inc = w > new_width ? w - new_width : 0;
	size_t dest_inc = w < new_width ? new_width - w : 0;
	while(rows) {
		*dest++ = *src++;
		++col;
		if(col == min_width) {
			if(--rows == 0) {
				break;
			}
			col = 0;
			src += src_inc;
			dest += dest_inc;
		}
	}
	pixels.swap(new_pixels);
	w = new_width;
	h = new_height;
	return true;
}

bool Pixmap::fill(unsigned index)
{
	if(index < palette.size()) {
		std::fill(pixels.begin(), pixels.end(), index);
		return true;
	}
	return false;
}

bool Pixmap::set_pixel(unsigned x, unsigned y, unsigned index)
{
	if(valid(x, y) && index < palette.size()) {
		pixels[x + y * w] = index;
		return true;
	}
	return false;
}

void run_floodfill(std::vector<unsigned> & pixels, unsigned width, unsigned height, unsigned x, unsigned y, unsigned what_index, unsigned to_what_index, int counter = 2e5)
{
	if(counter < 0) {
		return;
	}
	if(pixels[x + y * width] == to_what_index) {
		return;
	}
	if(pixels[x + y * width] != what_index) {
		return;
	}
	pixels[x + y * width] = to_what_index;
	if(x > 0) run_floodfill(pixels, width, height, x - 1, y, what_index, to_what_index, counter - 1);
	if(x < width - 1) run_floodfill(pixels, width, height, x + 1, y, what_index, to_what_index, counter - 1);
	if(y > 0) run_floodfill(pixels, width, height, x, y - 1, what_index, to_what_index, counter - 1);
	if(y < height - 1) run_floodfill(pixels, width, height, x, y + 1, what_index, to_what_index, counter - 1);
}

bool Pixmap::floodfill(unsigned x, unsigned y, unsigned index)
{
	if(valid(x, y) && index < palette.size()) {
		run_floodfill(pixels, w, h, x, y, pixel(x, y), index);
		return true;
	}
	return false;
}

unsigned Pixmap::add_color(Pixmap::Color new_color)
{
	palette.push_back(new_color);
	return palette.size() - 1;
}

bool Pixmap::set_color(unsigned index, Pixmap::Color new_color)
{
	if(index < palette.size()) {
		palette[index] = new_color;
		return true;
	}
	return false;
}

bool Pixmap::is_transparent_color(unsigned index) const
{
	if(index < palette.size()) {
		return palette[index].transparent;
	}
	return false;
}

