#pragma once
#include "font.h"
#include <chthon/pixmap.h>
#include <chthon/point.h>
#include <SDL2/SDL.h>

class PixelWidget {
public:
	PixelWidget(const std::string & imageFileName, int width = 0, int height = 0);
	virtual ~PixelWidget();

	int exec();
protected:
	void update();
	virtual void keyPressEvent(SDL_KeyboardEvent * event);
	void close();
private:
	SDL_Renderer * renderer;
	bool quit;
	int zoomFactor;
	Chthon::Point canvasShift;
	Chthon::Point cursor, oldCursor;
	uint color;
	std::string fileName;
	Chthon::Pixmap canvas;
	int mode;
	std::string colorEntered;
	bool wholeScreenChanged;
	bool do_draw_grid;
	Chthon::Point selection_start;
	SDL_Rect selection;
	SDL_Rect rect;
	SDL_Texture * dot_h;
	SDL_Texture * dot_v;
	Font font;

	void switch_draw_grid();
	Chthon::Pixmap::Color indexToRealColor(uint index);
	uint indexAtPos(const Chthon::Point & pos);
	void floodFill();
	void zoomIn();
	void zoomOut();
	void shiftCanvas(const Chthon::Point & shift, int speed = 1);
	void centerCanvas();
	void shiftCursor(const Chthon::Point & shift, int speed = 1);
	void putColorAtCursor();
	void takeColorUnderCursor();
	void startColorInput();
	void endColorInput();
	void pickNextColor();
	void pickPrevColor();
	void save();
	void startCopyMode();
	void startPasteMode();
	void drawCursor(const SDL_Rect & rect);
	void pasteSelection();
	void draw_pixel(const Chthon::Point & topLeft, const Chthon::Point & pos);
	void drawGrid(const Chthon::Point & topLeft);
	void recreate_dot_textures();
};
