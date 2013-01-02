#pragma once
#include <QtGui/QMainWindow>
#include "ui_mainwindow.h"

class PixelWidget;

class MainWindow : public QMainWindow {
	Q_OBJECT
	Q_DISABLE_COPY(MainWindow);
public:
	MainWindow(QWidget * parent = 0);
	virtual ~MainWindow() {}
private slots:
	void on_open_clicked();
	void on_save_clicked();
	void on_zoomIn_clicked();
	void on_zoomOut_clicked();
	void on_color_clicked();
private:
	Ui::MainWindow ui;
	PixelWidget * view;
};
