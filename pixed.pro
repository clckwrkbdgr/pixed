OBJECTS_DIR = tmp
QT -= gui
MOC_DIR = tmp
UI_DIR = tmp
LIBS += -lchthon -lSDL2

SOURCES += main.cpp
modules = pixelwidget qgetopt

for(module, modules) {
	HEADERS += $${module}.h
	SOURCES += $${module}.cpp
}
