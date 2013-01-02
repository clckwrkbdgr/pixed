#include <QtGui/QLabel>
#include <QtGui/QFileDialog>
#include "mainwindow.h"

MainWindow::MainWindow(QWidget * parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	view = new QLabel();
	ui.scrollArea->setWidget(view);
}

void MainWindow::on_open_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open image..."));
	if(fileName.isEmpty())
		return;

	QPixmap image(fileName);
	view->setScaledContents(true);
	view->setPixmap(image);
}

void MainWindow::on_save_clicked()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Open image..."));
	if(fileName.isEmpty())
		return;

	view->pixmap()->save(fileName);
}

