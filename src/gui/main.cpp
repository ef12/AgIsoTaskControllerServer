//================================================================================================
/// @file main.cpp
///
/// @brief Entry point: wires the bridge and models into QML and shows the main window.
//================================================================================================
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

#include "TcBridge.hpp"

int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);
	app.setApplicationName("AgIsoTaskControllerServer");
	app.setOrganizationName("Open-Agriculture");

	agisotc::TcBridge bridge;

	QQmlApplicationEngine engine;
	engine.rootContext()->setContextProperty("bridge", &bridge);
	engine.rootContext()->setContextProperty("clientModel", bridge.clientModel());
	engine.rootContext()->setContextProperty("ddopModel", bridge.ddopModel());
	engine.rootContext()->setContextProperty("valueModel", bridge.valueModel());
	engine.rootContext()->setContextProperty("logModel", bridge.logModel());

	engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
	if (engine.rootObjects().isEmpty())
	{
		return -1;
	}
	return app.exec();
}
