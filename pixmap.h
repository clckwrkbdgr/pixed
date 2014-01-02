#pragma once
#include <stdint.h>
#include <vector>

class Pixmap {
public:
	typedef uint32_t Color;

	Pixmap(unsigned w, unsigned h, unsigned palette_size = 1);
	bool valid(unsigned x, unsigned y) const;
	unsigned width() const;
	unsigned height() const;
	unsigned pixel(unsigned x, unsigned y) const;

	bool resize(unsigned new_width, unsigned new_height);
	bool fill(unsigned index);
	bool set_pixel(unsigned x, unsigned y, unsigned index);
	bool floodfill(unsigned x, unsigned y, unsigned index);

	unsigned color_count() const;
	Color color(unsigned index) const;

	unsigned add_color(Color new_color);
	bool set_color(unsigned index, Color new_color);
private:
	unsigned w, h;
	std::vector<unsigned> pixels;
	std::vector<Color> palette;
};
