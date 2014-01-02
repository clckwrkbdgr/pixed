#include "pixmap.h"

Pixmap::Pixmap(unsigned w, unsigned h, unsigned palette_size)
{
}

unsigned Pixmap::width() const
{
	return 0;
}

unsigned Pixmap::height() const
{
	return 0;
}

unsigned Pixmap::pixel(unsigned x, unsigned y) const
{
	return 0;
}

Pixmap::Color Pixmap::pixel_color(unsigned x, unsigned y) const
{
	return 0;
}

unsigned Pixmap::color_count() const
{
	return 0;
}

Pixmap::Color Pixmap::color(unsigned index) const
{
	return 0;
}

void Pixmap::resize(unsigned new_width, unsigned new_height)
{
}

void Pixmap::fill(unsigned index)
{
}

void Pixmap::set_pixel(unsigned x, unsigned y, unsigned index)
{
}

void Pixmap::set_pixel_color(unsigned x, unsigned y, Pixmap::Color new_color)
{
}

void Pixmap::floodfill(unsigned x, unsigned y, unsigned index)
{
}

void Pixmap::resize_palette(unsigned new_size)
{
}

unsigned Pixmap::add_color(Pixmap::Color new_color)
{
	return 0;
}

void Pixmap::set_color(unsigned index, Pixmap::Color new_color)
{
}

