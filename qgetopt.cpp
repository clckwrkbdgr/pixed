/** QGetopt v0.1.2
 * Source file: class implementation.
 */
#include "qgetopt.h"
#include <QtCore/QStringList>
#include <QtCore/QCoreApplication>
#include <QtCore/QVector>
namespace Getopt {
#include <getopt.h>
}

QGetopt::Option::Option()
	: withArg(false), count(0)
{
}

QGetopt::Option::Option(const QChar & _option, int _withArg)
	: option(_option), withArg(_withArg), count(1)
{
}

QGetopt::Option::Option(const QString & _longOption, int _withArg)
	: longOption(_longOption), withArg(_withArg), count(1)
{
}

QGetopt::Option::Option(const QChar & _option, const QString & _longOption, int _withArg)
	: option(_option), longOption(_longOption), withArg(_withArg), count(1)
{
}

bool QGetopt::Option::operator==(const Option & other) const
{
	if(option.isNull() || other.option.isNull()) {
		if(longOption.isNull()) {
			return false;
		}
		return longOption == other.longOption;
	}
	if(option == other.option) {
		if(longOption.isNull() || other.longOption.isNull()) {
			return true;
		}
		return longOption == other.longOption;
	}
	return false;
}

QGetopt & QGetopt::addOption(const QChar & shortOption)
{
	options << Option(shortOption);
	return * this;
}

QGetopt & QGetopt::addOption(const QString & longOption)
{
	options << Option(QChar(), longOption);
	return * this;
}

QGetopt & QGetopt::addOption(const QChar & shortOption, const QString & longOption)
{
	options << Option(shortOption, longOption);
	return * this;
}

QGetopt & QGetopt::addOptionWithArg(const QChar & shortOption)
{
	options << Option(shortOption, QString(), true);
	return * this;
}

QGetopt & QGetopt::addOptionWithArg(const QString & longOption)
{
	options << Option(QChar(), longOption, true);
	return * this;
}

QGetopt & QGetopt::addOptionWithArg(const QChar & shortOption, const QString & longOption)
{
	options << Option(shortOption, longOption, true);
	return * this;
}

// Helper struct.
// Aquires QStringList and creates an array of char*.
// So it can be put in getopt.
// Is a RAII, so resources allocated will be freed on scope exit.
// Has standard argc and argv fields.
struct Args {
	int argc;
	char ** argv;
	Args(const QStringList & arguments)
	{
		argc = arguments.count();
		argv = new char*[argc];
		for(int i = 0; i < argc; ++i) {
			QByteArray data = arguments[i].toAscii();
			argv[i] = new char[data.size() + 1];
			strcpy(argv[i], data.constData());
		}
	}
	~Args()
	{
		for(int i = 0; i < argc; ++i) {
			delete[] argv[i];
		}
		delete[] argv;
	}
};

QString QGetopt::getOptionString() const
{
	QString optionString;
	foreach(const Option & option, options) {
		if(option.option.isNull()) {
			continue;
		}
		optionString += option.option;
		if(option.withArg) {
			optionString += ":";
		}
	}
	return optionString;
}

// Helper struct for creating array of long options for getopt_long().
// Is also a RAII, so any allocated memory will be freed.
struct LongOpts {
	QVector<Getopt::option> longopts;

	LongOpts(const QList<QGetopt::Option> & options)
	{
		foreach(const QGetopt::Option & option, options) {
			if(option.longOption.isEmpty()) {
				continue;
			}

			QByteArray text = option.longOption.toLocal8Bit();
			text.append(char(0));
			char * s = new char[text.size()];
			strcpy(s, text.constData());

			longopts.append(Getopt::option());
			longopts.last().name = s;
			longopts.last().has_arg = option.withArg ? required_argument : no_argument;
			longopts.last().flag = 0;
			longopts.last().val = 0;
		}
		longopts.append(Getopt::option());
		longopts.last().name = 0;
		longopts.last().has_arg = 0;
		longopts.last().flag = 0;
		longopts.last().val = 0;
	}
	~LongOpts()
	{
		for(int i = 0; i < longopts.size(); ++i) {
			delete[] longopts[i].name;
		}
	}
};

QGetopt & QGetopt::parseApplicationArguments()
{
	return parse(QCoreApplication::arguments());
}

QGetopt & QGetopt::parse(const QStringList & arguments)
{
	Args args(arguments);
	QByteArray optionString = getOptionString().toLocal8Bit();
	LongOpts longopts(options);
	
	foundOptions.clear();
	nonArgs.clear();
	Getopt::optind = 0;
	Getopt::opterr = 0;

	QChar shortOption;
	int indexopt = 0;
	while((shortOption = Getopt::getopt_long(args.argc, args.argv, optionString.data(), longopts.longopts.constData(), &indexopt)) != -1) {
		bool isLongOption = shortOption.isNull();
		bool isError = shortOption == '?';
		QString longOption = (isLongOption || isError) ? QString(longopts.longopts[indexopt].name) : QString();

		if(!isLongOption && isError) {
			Option unknownOption = Option(QChar(Getopt::optopt));
			int index = options.indexOf(unknownOption);
			if(index > -1) {
				throw NoArgException(options[index]);
			}
			if(unknownOption) {
				throw UnknownOptionException(unknownOption);
			}
			longOption = args.argv[Getopt::optind - 1];
			if(longOption.startsWith("--")) {
				longOption.remove(0, 2);
			}
			index = options.indexOf(Option(longOption));
			if(index < 0) {
				throw UnknownOptionException(Option(longOption));
			}
			throw NoArgException(options[index]);
		}

		Option currentOption = isLongOption ? Option(longOption) : Option(shortOption);
		Option & option = getOption(currentOption);
		if(option) {
			option.count++;
			if(Getopt::optarg) {
				option.args << QString(Getopt::optarg);
			}
		} else {
			int index = options.indexOf(currentOption);
			Option newOption = options[index];
			if(Getopt::optarg) {
				newOption.args << QString(Getopt::optarg);
			}
			foundOptions << newOption;
		}
	}

	for(int i = Getopt::optind; i < args.argc; ++i) {
		nonArgs << QString(args.argv[i]);
	}
	return * this;
}

const QGetopt::Option & QGetopt::getOption(const Option & option) const
{
	int index = foundOptions.indexOf(option);
	if(index == -1) {
		static Option result;
		return result;
	}
	return foundOptions[index];
}

QGetopt::Option & QGetopt::getOption(const Option & option)
{
	int index = foundOptions.indexOf(option);
	if(index == -1) {
		static Option result;
		result = Option();
		return result;
	}
	return foundOptions[index];
}

bool QGetopt::hasOption(const QChar & shortOption) const
{
	return foundOptions.contains(Option(shortOption));
}

bool QGetopt::hasOption(const QString & longOption) const
{
	return foundOptions.contains(Option(longOption));
}

const QString & QGetopt::getArg(const QChar & shortOption) const
{
	return getOption(Option(shortOption)).args.first();
}

const QString & QGetopt::getArg(const QString & longOption) const
{
	return getOption(Option(longOption)).args.first();
}

const QStringList & QGetopt::getArgs(const QChar & shortOption) const
{
	return getOption(Option(shortOption)).args;
}

const QStringList & QGetopt::getArgs(const QString & longOption) const
{
	return getOption(Option(longOption)).args;
}

int QGetopt::count(const QChar & shortOption) const
{
	return getOption(Option(shortOption)).count;
}

int QGetopt::count(const QString & longOption) const
{
	return getOption(Option(longOption)).count;
}

const QStringList & QGetopt::getNonArgs() const
{
	return nonArgs;
}

