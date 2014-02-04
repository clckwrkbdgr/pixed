#pragma once
#include <chthon/pixmap.h>
#include <QtCore/QString>
#include <QtCore/QSize>
#include <QtCore/QPoint>
#include <QtCore/QRect>
#include <SDL2/SDL.h>

class PixelWidget {
public:
	PixelWidget(const QString & imageFileName, const QSize & newSize = QSize());
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
	QPoint canvasShift;
	QPoint cursor, oldCursor;
	uint color;
	QString fileName;
	Chthon::Pixmap canvas;
	int mode;
	QString colorEntered;
	bool wholeScreenChanged;
	bool do_draw_grid;
	QPoint selection_start;
	QRect selection;
	SDL_Rect rect;

	void switch_draw_grid();
	Chthon::Pixmap::Color indexToRealColor(uint index);
	uint indexAtPos(const QPoint & pos);
	void floodFill();
	void zoomIn();
	void zoomOut();
	void shiftCanvas(const QPoint & shift, int speed = 1);
	void centerCanvas();
	void shiftCursor(const QPoint & shift, int speed = 1);
	void putColorAtCursor();
	void takeColorUnderCursor();
	void startColorInput();
	void endColorInput();
	void pickNextColor();
	void pickPrevColor();
	void save();
	void startCopyMode();
	void startPasteMode();
	void drawCursor(const QRect & rect);
	void pasteSelection();
	void draw_pixel(const QPoint & topLeft, const QPoint & pos);
	void drawGrid(const QPoint & topLeft);
};
