#pragma once
#include <QtGui/QWidget>

class PixelWidget : public QWidget {
	Q_OBJECT
	Q_DISABLE_COPY(PixelWidget);
public:
	PixelWidget(const QString & imageFileName, QWidget * parent = 0);
	virtual ~PixelWidget();
protected:
	virtual void paintEvent(QPaintEvent*);
	virtual void keyPressEvent(QKeyEvent*);
private:
	int zoomFactor;
	QPoint canvasShift;
	QPoint cursor, oldCursor;
	uint color;
	QString fileName;
	QImage canvas;
	bool colorInputMode;
	QString colorEntered;
	bool wholeScreenChanged;
	bool do_draw_grid;

	void switch_draw_grid();
	QColor indexToRealColor(uint index);
	uint indexAtPos(const QPoint & pos);
	void floodFill();
	void zoomIn();
	void zoomOut();
	void shiftCanvas(const QPoint & shift);
	void centerCanvas();
	void shiftCursor(const QPoint & shift);
	void putColorAtCursor();
	void takeColorUnderCursor();
	void startColorInput();
	void endColorInput();
	void pickNextColor();
	void pickPrevColor();
};
