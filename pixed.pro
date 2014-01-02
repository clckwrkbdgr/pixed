OBJECTS_DIR = tmp
MOC_DIR = tmp
UI_DIR = tmp

PIXMAP_TEST {
	DEFINES += PIXMAP_TEST
	QT += testlib
	SOURCES += pixmap_test.cpp
	modules += pixmap
} else {
	SOURCES += main.cpp
	modules = pixmap pixelwidget qgetopt
}

for(module, modules) {
	HEADERS += $${module}.h
	SOURCES += $${module}.cpp
}
