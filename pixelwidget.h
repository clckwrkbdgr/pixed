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
	bool explicitCursor;
	int zoomFactor;
	QPoint canvasShift;
	QPoint cursor;
	uint color;
	QString fileName;
	QImage canvas;
	bool colorInputMode;
	QString colorEntered;

	QColor indexToRealColor(uint index);
	uint indexAtPos(const QPoint & pos);
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
};
