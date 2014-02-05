#include "pixelwidget.h"
#include <algorithm>
#include <iostream>
#include <getopt.h>

struct Options {
	int width, height;
	bool hasSize;
	std::string filename;
	Options() : width(0), height(0), hasSize(false) {}
	bool parse(int argc, char ** argv);
	bool printUsage();
};

bool Options::printUsage()
{
	static const std::string usage = 
			"Simple pixel graphic editor.\n"
			"Usage: pixed [-w WIDTH -h HEIGHT] FILENAME.xpm\n"
			"\t-w: set width for new image.\n"
			"\t-h: set height for new image.\n"
			"When width and height are specified, file is created anew.\n"
			"When no width and height are supplied, file is loaded.\n"
			"Only XPM images are recognized.\n"
			;
	std::cerr << usage;
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
	std::string width_string, height_string;
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
				return printUsage();
		}
	}
	bool hasWidthOrHeight = has_width || has_height;
	hasSize = has_width && has_height;
	if(hasWidthOrHeight && !hasSize) {
		return printUsage();
	}
	if(hasSize) {
		width = atoi(width_string.c_str());
		if(width <= 0) {
			return printUsage();
		}
		height = atoi(height_string.c_str());
		if(height <= 0) {
			return printUsage();
		}
	}
	if((argc - optind) < 1) {
		return printUsage();
	}
	filename = argv[optind];

	std::string lower_filename = filename;
	std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
	if(lower_filename.substr(lower_filename.size() - 4) != ".xpm") {
		return printUsage();
	}
	return true;
}

int main(int argc, char ** argv)
{
	Options options;
	if(!options.parse(argc, argv)) {
		return 1;
	}

	PixelWidget widget(options.filename, options.width, options.height);
	return widget.exec();
}
