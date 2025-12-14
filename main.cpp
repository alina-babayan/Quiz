#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "DatabaseManager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    DatabaseManager db;
    db.initDatabase();
    engine.rootContext()->setContextProperty("DB", &db);

    engine.load(QUrl(QStringLiteral("qrc:/new/prefix1/Main.qml")));
    if (engine.rootObjects().isEmpty()) return -1;

    return app.exec();
}
