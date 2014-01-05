#include "pixmap.h"
#include <QtDebug>
#include <sstream>
#include <algorithm>

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

Pixmap::Color Pixmap::Color::from_rgb(uint32_t color)
{
	return Color((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
}

bool Pixmap::Color::operator==(const Color & other) const
{
	return (transparent == other.transparent) || (r == other.r && g == other.g && b == other.b);
}

		
Pixmap::Pixmap(const std::string & xpm_data)
	: w(1), h(1), pixels(1, 0), palette(1)
{
	std::vector<std::string> lines = load_from_xpm_data(xpm_data);
	load_from_xpm_lines(lines);
}

Pixmap::Pixmap(const std::vector<std::string> & xpm_lines)
	: w(1), h(1), pixels(1, 0), palette(1)
{
	load_from_xpm_lines(xpm_lines);
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


std::vector<std::string> Pixmap::load_from_xpm_data(const std::string & xpm_data)
{
	std::istringstream iss(xpm_data);
	std::vector<std::string> tokens;
	std::string token;
	while(std::getline(iss, token, '\n')) {
		tokens.push_back(token);
	}
	std::vector<std::string> result;
	for(std::vector<std::string>::const_iterator it = tokens.begin(); it != tokens.end(); ++it) {
		const std::string & line = *it;
		size_t first = line.find('"');
		size_t second = line.find('"', first + 1);
		while(first != std::string::npos && second != std::string::npos) {
			result.push_back(line.substr(first + 1, second - first - 1));

			first = line.find('"', second + 1);
			second = line.find('"', first + 1);
		}
	}
	return result;
}

void Pixmap::load_from_xpm_lines(const std::vector<std::string> & xpm_lines)
{
	if(xpm_lines.empty()) {
		throw Exception("Value line is missing");
	}
	std::vector<std::string>::const_iterator line = xpm_lines.begin();
	std::istringstream in(*line);
	++line;
	std::vector<std::string> values;
	std::copy(std::istream_iterator<std::string>(in),
			std::istream_iterator<std::string>(),
			std::back_inserter<std::vector<std::string> >(values));
	if(values.size() != 4) {
		throw Exception("Value line should be in format '<width> <height> <color_count> <char_per_pixel>'");
	}

	w = atoi(values[0].c_str());
	h = atoi(values[1].c_str());
	unsigned colors = atoi(values[2].c_str());
	unsigned cpp = atoi(values[3].c_str());
	if(w == 0 || h == 0 || colors == 0 || cpp == 0) {
		throw Exception("Values in value line should be integers and non-zero.");
	}

	std::map<std::string, unsigned> color_names;
	palette.clear();
	while(colors --> 0) {
		if(line == xpm_lines.end()) {
			throw Exception("Color lines are missing or not enough");
		}
		std::string color_name = line->substr(0, cpp);
		if(line->size() <= cpp || (*line)[cpp] != ' ') {
			throw Exception("Color char should be followed by space in color table.");
		}
		std::istringstream color_in(line->substr(cpp + 1));
		std::string key;
		std::string value;
		color_in >> key;
		if(!color_in) {
			throw Exception("Color key is missing.");
		}
		if(key != "c") {
			throw Exception("Only color key 'c' is supported.");
		}
		color_in >> value;
		if(!color_in) {
			throw Exception("Color value is missing.");
		}
		if(color_names.count(color_name) > 0) {
			throw Exception("Color <" + color_name + "> was found more than once.");
		}
		value = value.substr(1);
		bool is_zero = true;
		for(std::string::const_iterator it = value.begin(); it != value.end(); ++it) {
			if(*it != '0') {
				is_zero = false;
				break;
			}
		}
		unsigned color_value = strtoul(value.c_str(), 0, 16);
		if(color_value == 0 && !is_zero) {
			throw Exception("Color value is invalid.");
		}
		Color color = Color::from_rgb(color_value);
		color_names[color_name] = add_color(color);
		++line;
	}

	pixels.clear();
	unsigned rows = h;
	while(rows --> 0) {
		if(line == xpm_lines.end()) {
			throw Exception("Pixel rows are missing or not enough");
		}
		if(line->size() % cpp != 0) {
			throw Exception("Pixel value in a row is broken.");
		} else if(line->size() < cpp * w) {
			throw Exception("Pixel row is too small.");
		} else if(line->size() > cpp * w) {
			throw Exception("Pixel row is too large.");
		}
		for(unsigned col = 0; col < w; ++col) {
			std::string pixel = line->substr(col * cpp, cpp);
			if(color_names.count(pixel) == 0) {
				throw Exception("Pixel value is invalid.");
			}
			pixels.push_back(color_names[pixel]);
		}
		++line;
	}
	if(line != xpm_lines.end()) {
		throw Exception("Extra pixel rows are found.");
	}
}

std::string Pixmap::save() const
{
	return "";
}
