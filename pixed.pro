OBJECTS_DIR = tmp
MOC_DIR = tmp
UI_DIR = tmp
LIBS += -lchthon

SOURCES += main.cpp
modules = pixelwidget qgetopt

for(module, modules) {
	HEADERS += $${module}.h
	SOURCES += $${module}.cpp
}
