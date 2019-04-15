TEMPLATE = app

include($$PWD/../sstd_library/sstd_build_path.pri)
include($$PWD/../sstd_library/cplusplus_basic.pri)

QT -= gui
QT -= core

SOURCES += $$PWD/main.cpp

DESTDIR = $$SSTD_LIBRARY_OUTPUT_PATH

DEFINES += CURRENT_DEBUG_PATH=\\\"$$PWD\\\"

