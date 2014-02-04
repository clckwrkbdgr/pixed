#include "pixelwidget.h"
#include "qgetopt.h"
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>

struct Options {
	int width, height;
	bool hasSize;
	QString filename;
	Options() : width(0), height(0), hasSize(false) {}
	bool parse();
	bool printUsage();
};

bool Options::printUsage()
{
	static const QString usage = QObject::tr(
			"Simple pixel graphic editor.\n"
			"Usage: pixed [-w WIDTH -h HEIGHT] FILENAME.xpm\n"
			"\t-w: set width for new image.\n"
			"\t-h: set height for new image.\n"
			"When width and height are specified, file is created anew.\n"
			"When no width and height are supplied, file is loaded.\n"
			"Only XPM images are recognized.\n"
			);
	QTextStream out(stdout);
	out << usage;
	return false;
}

bool Options::parse()
{
	QGetopt getopt;
	getopt.addOptionWithArg('w', "width").addOptionWithArg('h', "height");
	try {
		getopt.parseApplicationArguments();
	} catch(QGetopt::GetoptException & e) {
		return printUsage();
	}
	bool hasWidthOrHeight = getopt.hasOption('w') || getopt.hasOption('h');
	hasSize = getopt.hasOption('w') && getopt.hasOption('h');
	if(hasWidthOrHeight && !hasSize) {
		return printUsage();
	}
	if(hasSize) {
		bool ok = false;
		width = getopt.getArg('w').toInt(&ok);
		if(!ok || width <= 0) {
			return printUsage();
		}
		height = getopt.getArg('h').toInt(&ok);
		if(!ok || height <= 0) {
			return printUsage();
		}
	}
	if(getopt.getNonArgs().count() < 1) {
		return printUsage();
	}
	filename = getopt.getNonArgs().first();
	if(!filename.toLower().endsWith(".xpm")) {
		return printUsage();
	}
	return true;
}

int main(int argc, char ** argv)
{
	QCoreApplication app(argc, argv);
	app.setOrganizationName("kp580bm1");
	app.setApplicationName("pixed");

	Options options;
	if(!options.parse()) {
		return 1;
	}

	PixelWidget widget(options.filename, QSize(options.width, options.height));
	return widget.exec();
}
