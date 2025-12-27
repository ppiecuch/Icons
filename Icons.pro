TEMPLATE = app
QT += widgets svg sql
CONFIG += app_bundle sdk_no_version_check

VERSION = 1.2
BUILD = 69

DEFINES += APP_VERSION=\\\"$$VERSION\\\" APP_BUILD=$$BUILD

QMAKE_MACOSX_DEPLOYMENT_TARGET = 12

CONFIG(c++17): CXX = -c17
else:CONFIG(c++14): CXX = -c14
else:CONFIG(c++11): CXX = -c11
CONFIG(debug, debug|release): DBG = dbg
else: DBG = rel

CONFIG(release, debug|release): DEFINES += QT_NO_DEBUG_OUTPUT

T = $$lower($$TARGET)

DESTDIR = $$PWD/build-$$[QMAKE_XSPEC]$$CXX
SUBDIR = $${TEMPLATE}-$${T}.$${DBG}
OBJECTS_DIR = $$DESTDIR/$$SUBDIR/obj
MOC_DIR = $$DESTDIR/$$SUBDIR/ui
UI_DIR = $$DESTDIR/$$SUBDIR/ui
RCC_DIR = $$DESTDIR/$$SUBDIR/ui

COMPANY = KomSoft
BUNDLE_PREFIX = com.komsoft
RC_ICONS = appico.ico
ICON = appicon.icns


SOURCES = \
    icons.cpp \
    iconmodel.cpp \
    icongrid.cpp \
    extrawidgets.cpp

HEADERS = \
    icons.h \
    iconmodel.h \
    icongrid.h \
    extrawidgets.h

RESOURCES = icons.qrc
FORMS = icons.ui
INCLUDEPATH +=
LIBS +=

# Include library resources (generated icon data)
include(library/library.pri)

# Copy bitmap RCC files to app bundle (macOS)
macx {
    BITMAP_RCC.files = library/bitmap/oxygen.rcc library/bitmap/oxygen5.rcc
    BITMAP_RCC.path = Contents/Resources
    QMAKE_BUNDLE_DATA += BITMAP_RCC
}
