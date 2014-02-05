OBJECTS_DIR = tmp
QT -= gui
MOC_DIR = tmp
UI_DIR = tmp
LIBS += -lchthon -lSDL2
QMAKE_CXXFLAGS += -std=c++0x

SOURCES += main.cpp
modules = pixelwidget qgetopt font

for(module, modules) {
	HEADERS += $${module}.h
	SOURCES += $${module}.cpp
}
