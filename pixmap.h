#pragma once
#include <stdint.h>

class Pixmap {
public:
	typedef uint32_t Color;

	Pixmap(unsigned w, unsigned h, unsigned palette_size = 1);
	unsigned width() const;
	unsigned height() const;
	unsigned pixel(unsigned x, unsigned y) const;
	Color pixel_color(unsigned x, unsigned y) const;

	void resize(unsigned new_width, unsigned new_height);
	void fill(unsigned index);
	void set_pixel(unsigned x, unsigned y, unsigned index);
	void set_pixel_color(unsigned x, unsigned y, Color new_color);
	void floodfill(unsigned x, unsigned y, unsigned index);

	unsigned color_count() const;
	Color color(unsigned index) const;

	void resize_palette(unsigned new_size);
	unsigned add_color(Color new_color);
	void set_color(unsigned index, Color new_color);
};
