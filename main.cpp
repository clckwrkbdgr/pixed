#include <QtGui/QApplication>
#include <QtCore/QTextStream>
#include "pixelwidget.h"

int printUsage()
{
	QTextStream out(stdout);
	out << QObject::tr("Simple pixel graphic editor.") << endl;
	out << QObject::tr("Usage: pixed FILENAME") << endl;
	return -1;
}

int main(int argc, char ** argv)
{
	QApplication app(argc, argv);
	app.setOrganizationName("kp580bm1");
	app.setApplicationName("pixed");

	QStringList args = app.arguments();
	args.removeAt(0); // Program name;
	QString fileName;
	if(args.count() > 0) {
		fileName = args[0];
	} else {
		return printUsage();
	}

	PixelWidget widget(fileName);
	widget.showFullScreen();
	return app.exec();
}
