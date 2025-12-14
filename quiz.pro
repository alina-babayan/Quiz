QT += quick sql

CONFIG += c++17

SOURCES += \
    main.cpp \
    DatabaseManager.cpp

HEADERS += \
    DatabaseManager.h

RESOURCES += \
    qml.qrc

DISTFILES += \
    schema.sql
