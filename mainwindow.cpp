#include <QtGui/QLabel>
#include <QtGui/QFileDialog>
#include <QtGui/QColorDialog>
#include "pixelwidget.h"
#include "mainwindow.h"

MainWindow::MainWindow(QWidget * parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	view = new PixelWidget();
	ui.scrollArea->setWidget(view);
	view->show();
}

void MainWindow::on_open_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open image..."), QString(), tr("PNG Images (*.png)"));
	if(fileName.isEmpty())
		return;

	view->load(fileName);
}

void MainWindow::on_save_clicked()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save image..."), QString(), tr("PNG Images (*.png)"));
	if(fileName.isEmpty())
		return;

	view->save(fileName);
}

void MainWindow::on_zoomIn_clicked()
{
	view->zoomIn();
}

void MainWindow::on_zoomOut_clicked()
{
	view->zoomOut();
}

void MainWindow::on_color_clicked()
{
	QColor color = QColorDialog::getColor(view->currentColor(), this);
	if(color.isValid()) {
		view->setColor(color);
		ui.color->setText(color.name());
	}
}
