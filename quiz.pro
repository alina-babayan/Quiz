QT += quick sql

CONFIG += c++17

SOURCES += \
    main.cpp \
    DatabaseManager.cpp

HEADERS += \
    DatabaseManager.h

RESOURCES += \
    qml.qrc

# QML files are included in qml.qrc
# Example qml.qrc content:
# <RCC>
#   <qresource prefix="/">
#       <file>MainUI.qml</file>
#   </qresource>
# </RCC>

DISTFILES += \
    schema.sql
