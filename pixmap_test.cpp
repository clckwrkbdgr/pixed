//! Use `qmake "CONFIG += PIXMAP_TEST"` to build unit tests.
#include "pixmap.h"
#include <QtTest/QtTest>

template <class T, size_t N> size_t array_size(const T (&)[N]) { return N; }

class PixmapTest : public QObject {
	Q_OBJECT
private slots:
	void should_construct_color_from_argb();
	void should_construct_color_from_rgb();
	void should_construct_transparent_color_from_argb_with_zero_alpha();
	void should_get_transparent_rgba_as_transparent_black();
	void should_get_rgba_from_opaque_color();
	void should_make_palette_with_one_opaque_black_color_by_default();
	void should_fill_image_with_default_color_on_create();
	void should_fill_image_with_color();
	void should_resize_image();
	void should_not_resize_image_if_width_is_zero();
	void should_not_resize_image_if_height_is_zero();
	void should_keep_existing_pixels_when_resizing_image();
	void should_fill_new_pixels_with_default_index_when_resizing_image();
	void should_get_pixel_index();
	void should_set_pixel_index();
	void should_flood_fill_area();
	void should_change_palette_color_value();
	void should_add_new_color_to_palette();
	void should_consider_transparent_color_transparent();
	void should_consider_default_color_transparent();

	void should_load_pixmap_from_text_lines();
	void should_load_pixmap_from_xpm_file();
	void should_throw_exception_when_value_line_is_missing_in_xpm();
	void should_throw_exception_when_colour_lines_are_missing_or_not_enough_in_xpm();
	void should_throw_exception_when_pixel_lines_are_missing_or_not_enough_in_xpm();
	void should_throw_exception_when_value_count_is_not_four_in_xpm();
	void should_throw_exception_when_values_are_not_integer_in_xpm();
	void should_throw_exception_when_colours_are_repeated_in_xpm();
	void should_throw_exception_when_there_is_no_space_after_colour_in_xpm();
	void should_throw_exception_when_colour_key_is_not_c_in_xpm();
	void should_throw_exception_when_colour_key_is_missing_in_xpm();
	void should_throw_exception_when_colour_value_is_missing_in_xpm();
	void should_throw_exception_when_colour_value_is_invalid_in_xpm();
	void shoudl_throw_exception_when_pixel_row_length_is_too_small_in_xpm();
	void shoudl_throw_exception_when_pixel_row_length_is_too_large_in_xpm();
	void shoudl_throw_exception_when_pixel_row_count_is_too_large_in_xpm();
	void shoudl_throw_exception_when_pixel_is_broken_in_xpm();
	void shoudl_throw_exception_when_pixel_is_invalid_in_xpm();
	void should_save_pixmap_exactly_when_intact();
	void should_keep_format_of_values_when_saving_xpm();
	void should_keep_format_of_colours_when_saving_xpm();
	void should_insert_line_breaks_when_colours_are_added_when_saving_xpm();
	void should_lengthen_pixel_lines_when_width_is_increased_when_saving_xpm();
	void should_shorten_pixel_lines_when_width_is_decreased_when_saving_xpm();
	void should_add_new_lines_when_height_is_increased_when_saving_xpm();
	void should_remove_text_constants_when_height_is_decreased_when_saving_xpm();
};

void PixmapTest::should_construct_color_from_argb()
{
	Pixmap::Color c = Pixmap::Color::from_argb(0x00ff00ff);
	QVERIFY(c.transparent);
	QCOMPARE((int)c.r, 0);
	QCOMPARE((int)c.g, 0);
	QCOMPARE((int)c.b, 0);
}

void PixmapTest::should_construct_color_from_rgb()
{
	Pixmap::Color c = Pixmap::Color::from_rgb(0x00ff00ff);
	QVERIFY(!c.transparent);
	QCOMPARE((int)c.r, 255);
	QCOMPARE((int)c.g, 0);
	QCOMPARE((int)c.b, 255);
}

void PixmapTest::should_construct_transparent_color_from_argb_with_zero_alpha()
{
	Pixmap::Color c = Pixmap::Color::from_argb(0xf0ff00ff);
	QVERIFY(!c.transparent);
	QCOMPARE((int)c.r, 255);
	QCOMPARE((int)c.g, 0);
	QCOMPARE((int)c.b, 255);
}

void PixmapTest::should_get_transparent_rgba_as_transparent_black()
{
	Pixmap::Color c;
	QCOMPARE(c.argb(), 0u);
}

void PixmapTest::should_get_rgba_from_opaque_color()
{
	Pixmap::Color c(255, 0, 255);
	QCOMPARE(c.argb(), 0xffff00ff);
}

void PixmapTest::should_make_palette_with_one_opaque_black_color_by_default()
{
	Pixmap pixmap(2, 2);
	QCOMPARE(pixmap.color_count(), 1u);
	QCOMPARE(pixmap.color(0), Pixmap::Color(0, 0, 0));
}

void PixmapTest::should_fill_image_with_default_color_on_create()
{
	Pixmap pixmap(2, 2);
	for(unsigned x = 0; x < 2; ++x) {
		for(unsigned y = 0; y < 2; ++y) {
			QCOMPARE(pixmap.pixel(x, y), 0u);
		}
	}
}

void PixmapTest::should_fill_image_with_color()
{
	Pixmap pixmap(2, 2);
	unsigned color = pixmap.add_color(Pixmap::Color(255, 255, 255));
	pixmap.fill(color);
	for(unsigned x = 0; x < 2; ++x) {
		for(unsigned y = 0; y < 2; ++y) {
			QCOMPARE(pixmap.pixel(x, y), color);
		}
	}
}

void PixmapTest::should_resize_image()
{
	Pixmap pixmap(2, 2);
	pixmap.resize(4, 4);
	QCOMPARE(pixmap.width(), 4u);
	QCOMPARE(pixmap.height(), 4u);
}

void PixmapTest::should_not_resize_image_if_width_is_zero()
{
	Pixmap pixmap(2, 2);
	pixmap.resize(0, 4);
	QCOMPARE(pixmap.width(), 2u);
	QCOMPARE(pixmap.height(), 2u);
}

void PixmapTest::should_not_resize_image_if_height_is_zero()
{
	Pixmap pixmap(2, 2);
	pixmap.resize(4, 0);
	QCOMPARE(pixmap.width(), 2u);
	QCOMPARE(pixmap.height(), 2u);
}

void PixmapTest::should_keep_existing_pixels_when_resizing_image()
{
	Pixmap pixmap(2, 2, 4);
	pixmap.set_pixel(0, 0, 0);
	pixmap.set_pixel(1, 0, 1);
	pixmap.set_pixel(0, 1, 2);
	pixmap.set_pixel(1, 1, 3);
	pixmap.resize(4, 4);
	QCOMPARE(pixmap.pixel(0, 0), 0u);
	QCOMPARE(pixmap.pixel(1, 0), 1u);
	QCOMPARE(pixmap.pixel(0, 1), 2u);
	QCOMPARE(pixmap.pixel(1, 1), 3u);
}

void PixmapTest::should_fill_new_pixels_with_default_index_when_resizing_image()
{
	Pixmap pixmap(2, 2, 2);
	pixmap.set_pixel(0, 0, 1);
	pixmap.set_pixel(1, 0, 1);
	pixmap.set_pixel(0, 1, 1);
	pixmap.set_pixel(1, 1, 1);
	pixmap.resize(2, 3);
	QCOMPARE(pixmap.pixel(0, 2), 0u);
	QCOMPARE(pixmap.pixel(1, 2), 0u);
}

void PixmapTest::should_get_pixel_index()
{
	Pixmap pixmap(2, 2, 2);
	pixmap.set_pixel(0, 0, 1);
	QCOMPARE(pixmap.pixel(0, 0), 1u);
}

void PixmapTest::should_set_pixel_index()
{
	Pixmap pixmap(2, 2, 2);
	pixmap.set_pixel(0, 0, 1);
	QCOMPARE(pixmap.pixel(0, 0), 1u);
}

void PixmapTest::should_flood_fill_area()
{
	Pixmap pixmap(8, 8, 3);
	unsigned data[64] = {
		0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 1, 1, 1, 0, 0, 0, 
		0, 1, 0, 0, 0, 1, 0, 0, 
		0, 0, 1, 0, 0, 1, 0, 0, 
		0, 0, 0, 1, 0, 1, 0, 0, 
		0, 0, 1, 0, 0, 0, 1, 0, 
		0, 1, 0, 0, 0, 0, 1, 0, 
		0, 1, 0, 0, 0, 0, 1, 0, 
	};
	for(unsigned i = 0; i < 64; ++i) {
		pixmap.set_pixel(i % 8, i / 8, data[i]);
	}
	pixmap.floodfill(5, 5, 2);
	unsigned result[64] = {
		0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 1, 1, 1, 0, 0, 0, 
		0, 1, 2, 2, 2, 1, 0, 0, 
		0, 0, 1, 2, 2, 1, 0, 0, 
		0, 0, 0, 1, 2, 1, 0, 0, 
		0, 0, 1, 2, 2, 2, 1, 0, 
		0, 1, 2, 2, 2, 2, 1, 0, 
		0, 1, 2, 2, 2, 2, 1, 0, 
	};
	for(unsigned i = 0; i < 64; ++i) {
		QCOMPARE(pixmap.pixel(i % 8, i / 8), result[i]);
	}
}

void PixmapTest::should_change_palette_color_value()
{
	Pixmap pixmap(2, 2, 2);
	pixmap.set_color(1, Pixmap::Color(0, 255, 0));
	QCOMPARE(pixmap.color(1), Pixmap::Color(0, 255, 0));
}

void PixmapTest::should_add_new_color_to_palette()
{
	Pixmap pixmap(2, 2, 2);
	unsigned index = pixmap.add_color(Pixmap::Color(0, 255, 0));
	QCOMPARE(index, 2u);
	QCOMPARE(pixmap.color(2), Pixmap::Color(0, 255, 0));
}

void PixmapTest::should_consider_transparent_color_transparent()
{
	Pixmap pixmap(2, 2, 2);
	Pixmap::Color c(255, 255, 255);
	c.transparent = true;
	pixmap.set_color(0, c);
	QVERIFY(pixmap.is_transparent_color(0));
}

void PixmapTest::should_consider_default_color_transparent()
{
	Pixmap pixmap(2, 2, 2);
	pixmap.set_color(0, Pixmap::Color());
	QVERIFY(pixmap.is_transparent_color(0));
}

void PixmapTest::should_load_pixmap_from_text_lines()
{
	static const char * xpm[] = {
	"3 2 2 1",
	". c #ff0000",
	"# c #00ff00",
	"#.#",
	".#."
	};
	std::vector<std::string> xpm_lines(xpm, xpm + array_size(xpm));
	Pixmap pixmap(xpm_lines);
	QCOMPARE(pixmap.color_count(), 2u);
	QCOMPARE(pixmap.color(0), Pixmap::Color(255, 0, 0));
	QCOMPARE(pixmap.color(1), Pixmap::Color(0, 255, 0));
	QCOMPARE(pixmap.width(), 3u);
	QCOMPARE(pixmap.height(), 2u);
	QCOMPARE(pixmap.pixel(0, 0), 1u);
	QCOMPARE(pixmap.pixel(1, 0), 0u);
	QCOMPARE(pixmap.pixel(2, 0), 1u);
	QCOMPARE(pixmap.pixel(0, 1), 0u);
	QCOMPARE(pixmap.pixel(1, 1), 1u);
	QCOMPARE(pixmap.pixel(2, 1), 0u);
}

void PixmapTest::should_load_pixmap_from_xpm_file()
{
	static const char * xpm_data = 
		"static char * xpm[] = {\n"
		"\"3 2 2 1\",\n"
		"\". c #ff0000\",\n"
		"\"# c #00ff00\",\n"
		"\"#.#\",\n"
		"\".#.\"\n"
		"};\n"
		;
	Pixmap pixmap(xpm_data);
	QCOMPARE(pixmap.color_count(), 2u);
	QCOMPARE(pixmap.color(0), Pixmap::Color(255, 0, 0));
	QCOMPARE(pixmap.color(1), Pixmap::Color(0, 255, 0));
	QCOMPARE(pixmap.width(), 3u);
	QCOMPARE(pixmap.height(), 2u);
	QCOMPARE(pixmap.pixel(0, 0), 1u);
	QCOMPARE(pixmap.pixel(1, 0), 0u);
	QCOMPARE(pixmap.pixel(2, 0), 1u);
	QCOMPARE(pixmap.pixel(0, 1), 0u);
	QCOMPARE(pixmap.pixel(1, 1), 1u);
	QCOMPARE(pixmap.pixel(2, 1), 0u);
}

void PixmapTest::should_throw_exception_when_value_line_is_missing_in_xpm()
{
	std::vector<std::string> xpm_lines;
	try {
		Pixmap pixmap(xpm_lines);
		QFAIL("Exception wasn't thrown");
	} catch(const Pixmap::Exception & e) {
		QCOMPARE(e.what, std::string("Value line is missing"));
	}
}

void PixmapTest::should_throw_exception_when_colour_lines_are_missing_or_not_enough_in_xpm()
{
	static const char * xpm[] = {
	"3 2 2 1",
	". c #ff0000"
	};
	std::vector<std::string> xpm_lines(xpm, xpm + array_size(xpm));
	try {
		Pixmap pixmap(xpm_lines);
		QFAIL("Exception wasn't thrown");
	} catch(const Pixmap::Exception & e) {
		QCOMPARE(e.what, std::string("Color lines are missing or not enough"));
	}
}

void PixmapTest::should_throw_exception_when_pixel_lines_are_missing_or_not_enough_in_xpm()
{
	static const char * xpm[] = {
	"3 2 2 1",
	". c #ff0000",
	"# c #00ff00",
	"#.#"
	};
	std::vector<std::string> xpm_lines(xpm, xpm + array_size(xpm));
	try {
		Pixmap pixmap(xpm_lines);
		QFAIL("Exception wasn't thrown");
	} catch(const Pixmap::Exception & e) {
		QCOMPARE(e.what, std::string("Pixel rows are missing or not enough"));
	}
}

void PixmapTest::should_throw_exception_when_value_count_is_not_four_in_xpm()
{
	static const char * xpm[] = {
	"3 2",
	};
	std::vector<std::string> xpm_lines(xpm, xpm + array_size(xpm));
	try {
		Pixmap pixmap(xpm_lines);
		QFAIL("Exception wasn't thrown");
	} catch(const Pixmap::Exception & e) {
		QCOMPARE(e.what, std::string("Value line should be in format '<width> <height> <color_count> <char_per_pixel>'"));
	}
}

void PixmapTest::should_throw_exception_when_values_are_not_integer_in_xpm()
{
	static const char * xpm[] = {
	"3 2 a 1",
	};
	std::vector<std::string> xpm_lines(xpm, xpm + array_size(xpm));
	try {
		Pixmap pixmap(xpm_lines);
		QFAIL("Exception wasn't thrown");
	} catch(const Pixmap::Exception & e) {
		QCOMPARE(e.what, std::string("Values in value line should be integers and non-zero."));
	}
}

void PixmapTest::should_throw_exception_when_colours_are_repeated_in_xpm()
{
	static const char * xpm[] = {
	"3 2 2 1",
	". c #ff0000",
	". c #00ff00",
	};
	std::vector<std::string> xpm_lines(xpm, xpm + array_size(xpm));
	try {
		Pixmap pixmap(xpm_lines);
		QFAIL("Exception wasn't thrown");
	} catch(const Pixmap::Exception & e) {
		QCOMPARE(e.what, std::string("Color <.> was found more than once."));
	}
}

void PixmapTest::should_throw_exception_when_there_is_no_space_after_colour_in_xpm()
{
	static const char * xpm[] = {
	"3 2 2 1",
	".c #ff0000"
	};
	std::vector<std::string> xpm_lines(xpm, xpm + array_size(xpm));
	try {
		Pixmap pixmap(xpm_lines);
		QFAIL("Exception wasn't thrown");
	} catch(const Pixmap::Exception & e) {
		QCOMPARE(e.what, std::string("Color char should be followed by space in color table."));
	}
}

void PixmapTest::should_throw_exception_when_colour_key_is_not_c_in_xpm()
{
	static const char * xpm[] = {
	"3 2 2 1",
	". s #ff0000"
	};
	std::vector<std::string> xpm_lines(xpm, xpm + array_size(xpm));
	try {
		Pixmap pixmap(xpm_lines);
		QFAIL("Exception wasn't thrown");
	} catch(const Pixmap::Exception & e) {
		QCOMPARE(e.what, std::string("Only color key 'c' is supported."));
	}
}

void PixmapTest::should_throw_exception_when_colour_key_is_missing_in_xpm()
{
	static const char * xpm[] = {
	"3 2 2 1",
	". "
	};
	std::vector<std::string> xpm_lines(xpm, xpm + array_size(xpm));
	try {
		Pixmap pixmap(xpm_lines);
		QFAIL("Exception wasn't thrown");
	} catch(const Pixmap::Exception & e) {
		QCOMPARE(e.what, std::string("Color key is missing."));
	}
}

void PixmapTest::should_throw_exception_when_colour_value_is_missing_in_xpm()
{
	static const char * xpm[] = {
	"3 2 2 1",
	". c"
	};
	std::vector<std::string> xpm_lines(xpm, xpm + array_size(xpm));
	try {
		Pixmap pixmap(xpm_lines);
		QFAIL("Exception wasn't thrown");
	} catch(const Pixmap::Exception & e) {
		QCOMPARE(e.what, std::string("Color value is missing."));
	}
}

void PixmapTest::should_throw_exception_when_colour_value_is_invalid_in_xpm()
{
	static const char * xpm[] = {
	"3 2 2 1",
	". c invalid"
	};
	std::vector<std::string> xpm_lines(xpm, xpm + array_size(xpm));
	try {
		Pixmap pixmap(xpm_lines);
		QFAIL("Exception wasn't thrown");
	} catch(const Pixmap::Exception & e) {
		QCOMPARE(e.what, std::string("Color value is invalid."));
	}
}

void PixmapTest::shoudl_throw_exception_when_pixel_row_length_is_too_small_in_xpm()
{
	static const char * xpm[] = {
	"3 2 2 1",
	". c #ff0000",
	"# c #00ff00",
	"#.",
	".#"
	};
	std::vector<std::string> xpm_lines(xpm, xpm + array_size(xpm));
	try {
		Pixmap pixmap(xpm_lines);
		QFAIL("Exception wasn't thrown");
	} catch(const Pixmap::Exception & e) {
		QCOMPARE(e.what, std::string("Pixel row is too small."));
	}
}

void PixmapTest::shoudl_throw_exception_when_pixel_row_length_is_too_large_in_xpm()
{
	static const char * xpm[] = {
	"3 2 2 1",
	". c #ff0000",
	"# c #00ff00",
	"#.##",
	".#"
	};
	std::vector<std::string> xpm_lines(xpm, xpm + array_size(xpm));
	try {
		Pixmap pixmap(xpm_lines);
		QFAIL("Exception wasn't thrown");
	} catch(const Pixmap::Exception & e) {
		QCOMPARE(e.what, std::string("Pixel row is too large."));
	}
}

void PixmapTest::shoudl_throw_exception_when_pixel_row_count_is_too_large_in_xpm()
{
	static const char * xpm[] = {
	"3 2 2 1",
	". c #ff0000",
	"# c #00ff00",
	"#.#",
	"#.#",
	".##"
	};
	std::vector<std::string> xpm_lines(xpm, xpm + array_size(xpm));
	try {
		Pixmap pixmap(xpm_lines);
		QFAIL("Exception wasn't thrown");
	} catch(const Pixmap::Exception & e) {
		QCOMPARE(e.what, std::string("Extra pixel rows are found."));
	}
}

void PixmapTest::shoudl_throw_exception_when_pixel_is_broken_in_xpm()
{
	static const char * xpm[] = {
	"3 2 2 2",
	"a. c #ff0000",
	"b# c #00ff00",
	"b#a.b#",
	"a.b#a"
	};
	std::vector<std::string> xpm_lines(xpm, xpm + array_size(xpm));
	try {
		Pixmap pixmap(xpm_lines);
		QFAIL("Exception wasn't thrown");
	} catch(const Pixmap::Exception & e) {
		QCOMPARE(e.what, std::string("Pixel value in a row is broken."));
	}
}

void PixmapTest::shoudl_throw_exception_when_pixel_is_invalid_in_xpm()
{
	static const char * xpm[] = {
	"3 2 2 2",
	"a. c #ff0000",
	"b# c #00ff00",
	"b#a.b#",
	"a.b#a$"
	};
	std::vector<std::string> xpm_lines(xpm, xpm + array_size(xpm));
	try {
		Pixmap pixmap(xpm_lines);
		QFAIL("Exception wasn't thrown");
	} catch(const Pixmap::Exception & e) {
		QCOMPARE(e.what, std::string("Pixel value is invalid."));
	}
}

void PixmapTest::should_save_pixmap_exactly_when_intact()
{
	QFAIL("Not implemented");
}

void PixmapTest::should_keep_format_of_values_when_saving_xpm()
{
	QFAIL("Not implemented");
}

void PixmapTest::should_keep_format_of_colours_when_saving_xpm()
{
	QFAIL("Not implemented");
}

void PixmapTest::should_insert_line_breaks_when_colours_are_added_when_saving_xpm()
{
	QFAIL("Not implemented");
}

void PixmapTest::should_lengthen_pixel_lines_when_width_is_increased_when_saving_xpm()
{
	QFAIL("Not implemented");
}

void PixmapTest::should_shorten_pixel_lines_when_width_is_decreased_when_saving_xpm()
{
	QFAIL("Not implemented");
}

void PixmapTest::should_add_new_lines_when_height_is_increased_when_saving_xpm()
{
	QFAIL("Not implemented");
}

void PixmapTest::should_remove_text_constants_when_height_is_decreased_when_saving_xpm()
{
	QFAIL("Not implemented");
}

QTEST_MAIN(PixmapTest)
#include "pixmap_test.moc"

