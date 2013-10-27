#pragma once
/** QGetopt v0.1.2
 * Header file: class definition.
 */
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QList>

/** Wrapper class for GNU getopt (and getopt_long) functions.
 * Usage:
 * - specify options to check via addOption() and addOptionWithArg() calls.
 * - run parse() with argument list or parseApplicationArgs().
 * - check found options and it's arguments using hasOption(), getArg(), getArgs(), and count() methods.
 */
class QGetopt {
	struct Option;
public:
	/** Basic exception class for all getopt exceptions. */
	struct GetoptException {
		QChar option; /** Short option. Null if no short option specified. */
		QString longOption; /** Long option. Empty if no long option specified. */
		/** Creates exception for option o. */
		GetoptException(const Option & o) : option(o.option), longOption(o.longOption) {}
		virtual ~GetoptException() {}
	};
	/** No argument exception. Thrown when no argument specified for option with required arg. */
	struct NoArgException : GetoptException {
		/** Creates exception for option o. */
		NoArgException(const Option & o) : GetoptException(o) {}
	};
	/** Unknown option exception. Thrown when unknown option found (i.e. wasn't specified via addOption(). */
	struct UnknownOptionException : GetoptException {
		/** Creates exception for option o. */
		UnknownOptionException(const Option & o) : GetoptException(o) {}
	};

	QGetopt() {}
	virtual ~QGetopt() {}

	/** Adds short option to check. */
	QGetopt & addOption(const QChar & shortOption);
	/** Adds long option to check. */
	QGetopt & addOption(const QString & longOption);
	/** Adds option to check with short and long equivalent. */
	QGetopt & addOption(const QChar & shortOption, const QString & longOption);
	/** Adds short option to check with required argument. */
	QGetopt & addOptionWithArg(const QChar & shortOption);
	/** Adds long option to check with required argument. */
	QGetopt & addOptionWithArg(const QString & longOption);
	/** Adds option to check with short and long equivalent and required argument. */
	QGetopt & addOptionWithArg(const QChar & shortOption, const QString & longOption);

	/** Run options parsing.
	 * @param arguments - argument list. Could be retrieved from QCoreApplication object via arguments() method.
	 */
	QGetopt & parse(const QStringList & arguments);
	/** Run options parsing.
	 * Convinience method, takes arguments from QCoreApplication object implicitly.
	 */
	QGetopt & parseApplicationArguments();

	/** Returns true if shortOption was found.
	 * If option with both shortOption and some long option was specified,
	 * but only long option was present at argument list, this method will
	 * also return true, so as any other of methods of this group would do.
	 */
	bool hasOption(const QChar & shortOption) const;
	/** Returns true if longOption was found.
	 * If option with both shortOption and some long option was specified,
	 * but only long option was present at argument list, this method will
	 * also return true, so as any other of methods of this group would do.
	 */
	bool hasOption(const QString & longOption) const;
	/** Returns argument for shortOption.
	 * If option was found several times, returns argument from the first one.
	 */
	const QString & getArg(const QChar & shortOption) const;
	/** Returns argument for longOption.
	 * If option was found several times, returns argument from the first one.
	 */
	const QString & getArg(const QString & longOption) const;
	/** Returns arguments for shortOption for each occasion. */
	const QStringList & getArgs(const QChar & shortOption) const;
	/** Returns arguments for longOption for each occasion. */
	const QStringList & getArgs(const QString & longOption) const;
	/** Returns count of times when option was found. */
	int count(const QChar & shortOption) const;
	/** Returns count of times when option was found. */
	int count(const QString & longOption) const;

	/** Returns non-args entries (defined as in getopt). */
	const QStringList & getNonArgs() const;
private:
	struct Option {
		QChar option;
		QString longOption;
		bool withArg;
		QStringList args;
		int count;

		Option();
		explicit Option(const QChar & _option, int _withArg = false);
		explicit Option(const QString & _longOption, int _withArg = false);
		explicit Option(const QChar & _option, const QString & _longOption, int _withArg = false);
		operator bool() const { return !option.isNull() || !longOption.isEmpty(); }
		bool operator==(const Option & other) const;
		bool operator!=(const Option & other) const { return !(*this == other); }
	};
	QList<Option> options, foundOptions;
	QStringList nonArgs;

	Option & getOption(const Option & option);
	const Option & getOption(const Option & option) const;
	QString getOptionString() const;

	friend class LongOpts;
	friend class TestQGetopt;
};
