TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += c++17

SOURCES += \
        main.cpp

macx: CONFIG += sdk_no_version_check

unix{
    QMAKE_CXXFLAGS += -std=c++17 -Wall -Wpedantic
}
win32-msvc: QMAKE_CXXFLAGS += /std:c++14
win32-msvc: DEFINES += CRT_SECURE_NO_WARNINGS
win32-msvc: LIBS += -lole32

macx{ LIBS += -L$$PWD/portaudio/build_mac/ -lportaudio
INCLUDEPATH += $$PWD/portaudio/build_mac
DEPENDPATH += $$PWD/portaudio/build_mac
}



HEADERS += \
    cppaudio.hpp

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/portaudio/build/msvc/x64/release/ -lportaudio_x64
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/portaudio/build/msvc/x64/release/ -lportaudio_x64

INCLUDEPATH += $$PWD/portaudio/include
DEPENDPATH += $$PWD/portaudio/include
