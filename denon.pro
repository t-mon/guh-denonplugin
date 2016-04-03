include(plugins.pri)

TARGET = guh_deviceplugindenon

message(============================================)
message("Qt version: $$[QT_VERSION]")
message("Building $$deviceplugin$${TARGET}.so")


SOURCES += \
    deviceplugindenon.cpp \
    denonconnection.cpp

HEADERS += \
    deviceplugindenon.h \
    denonconnection.h
