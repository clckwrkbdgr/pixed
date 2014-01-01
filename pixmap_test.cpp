//! Use `qmake "CONFIG += PIXMAP_TEST"` to build unit tests.
#include "pixmap.h"
#include <QtTest/QtTest>

class PixmapTest : public QObject {
	Q_OBJECT
private slots:
	void should_fill_image_with_color();
	void should_resize_palette();
	void should_keep_existing_colours_when_resizing_palette();
	void should_fill_new_colours_with_default_color_when_resizing_palette();
	void should_resize_image();
	void should_keep_existing_pixels_when_resizing_image();
	void should_fill_new_pixels_with_default_index_when_resizing_palette();
	void should_get_pixel_color();
	void should_get_pixel_index();
	void should_set_pixel_index();
	void should_use_first_matching_index_when_setting_pixel_color();
	void should_add_color_if_not_exist_when_setting_pixel_color();
	void should_convert_color_to_string();
	void should_flood_fill_area();
	void should_not_flood_fill_if_colors_are_the_same();
	void should_change_palette_color_value();
	void should_add_new_color_to_palette();
};

void PixmapTest::should_fill_image_with_color()
{
}

void PixmapTest::should_resize_palette()
{
}

void PixmapTest::should_keep_existing_colours_when_resizing_palette()
{
}

void PixmapTest::should_fill_new_colours_with_default_color_when_resizing_palette()
{
}

void PixmapTest::should_resize_image()
{
}

void PixmapTest::should_keep_existing_pixels_when_resizing_image()
{
}

void PixmapTest::should_fill_new_pixels_with_default_index_when_resizing_palette()
{
}

void PixmapTest::should_get_pixel_color()
{
}

void PixmapTest::should_get_pixel_index()
{
}

void PixmapTest::should_set_pixel_index()
{
}

void PixmapTest::should_use_first_matching_index_when_setting_pixel_color()
{
}

void PixmapTest::should_add_color_if_not_exist_when_setting_pixel_color()
{
}

void PixmapTest::should_convert_color_to_string()
{
}

void PixmapTest::should_flood_fill_area()
{
}

void PixmapTest::should_not_flood_fill_if_colors_are_the_same()
{
}

void PixmapTest::should_change_palette_color_value()
{
}

void PixmapTest::should_add_new_color_to_palette()
{
}

QTEST_MAIN(PixmapTest)
#include "pixmap_test.moc"

