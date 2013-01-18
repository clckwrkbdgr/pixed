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
	enum RepaintReason { WHOLE_IMAGE_CHANGED = 0, INTERFACE_CHANGED, PIXEL_CHANGED, CURSOR_MOVED };

	bool explicitCursor;
	int zoomFactor;
	QPoint canvasShift;
	QPoint cursor, oldCursor;
	uint color;
	QString fileName;
	QImage canvas;
	bool colorInputMode;
	QString colorEntered;
	RepaintReason repaintReason;

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
	void toggleExplicitCursor();
	void startColorInput();
	void endColorInput();
	void pickNextColor();
	void pickPrevColor();
};
