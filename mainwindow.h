#pragma once
#include <QtGui/QWidget>

class MainWindow : public QWidget {
	Q_OBJECT
	Q_DISABLE_COPY(MainWindow);
public:
	MainWindow(QWidget * parent = 0);
	virtual ~MainWindow() {}
};
