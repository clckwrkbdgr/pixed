#include "pixelwidget.h"
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>
#include <getopt.h>

struct Options {
	int width, height;
	bool hasSize;
	QString filename;
	Options() : width(0), height(0), hasSize(false) {}
	bool parse(int argc, char ** argv);
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

bool Options::parse(int argc, char ** argv)
{
	static struct option long_options[] = {
		{"width", required_argument, 0, 'w'},
		{"height", required_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	int c;
	bool has_width = false;
	bool has_height = false;
	QString width_string, height_string;
	while((c = getopt_long(argc, argv, "w:h:", long_options, 0)) != -1) {
		switch(c) {
			case 'w':
				has_width = true;
				width_string = optarg;
				break;
			case 'h':
				has_height = true;
				height_string = optarg;
				break;
			case '?':
			default:
				printUsage();
				return false;
		}
	}
	bool hasWidthOrHeight = has_width || has_height;
	hasSize = has_width && has_height;
	if(hasWidthOrHeight && !hasSize) {
		return printUsage();
	}
	if(hasSize) {
		bool ok = false;
		width = width_string.toInt(&ok);
		if(!ok || width <= 0) {
			return printUsage();
		}
		height = height_string.toInt(&ok);
		if(!ok || height <= 0) {
			return printUsage();
		}
	}
	if((argc - optind) < 1) {
		return printUsage();
	}
	filename = argv[optind];
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
	if(!options.parse(argc, argv)) {
		return 1;
	}

	PixelWidget widget(options.filename, QSize(options.width, options.height));
	return widget.exec();
}
