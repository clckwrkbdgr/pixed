#pragma once
#include <QtGui/QWidget>

class PixelWidget : public QWidget {
	Q_OBJECT
	Q_DISABLE_COPY(PixelWidget);
public:
	PixelWidget(QWidget * parent = 0);
	virtual ~PixelWidget() {}

	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const;
public slots:
	void load(const QString & fileName);
	void save(const QString & fileName);
	void zoomIn();
	void zoomOut();
protected:
	virtual void paintEvent(QPaintEvent*);
private:
	int zoomFactor;
	QImage image;
};
