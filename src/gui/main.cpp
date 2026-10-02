//================================================================================================
/// @file main.cpp
///
/// @brief Entry point: wires the bridge and models into QML and shows the main window.
//================================================================================================
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <QVariantMap>

#include "TcBridge.hpp"

namespace
{
	/// @brief Reads the start-up options that preset the top bar, so the server can be started
	/// from a script. Options that are not given keep the GUI defaults.
	QVariantMap parse_startup_options(const QGuiApplication &app)
	{
		QCommandLineParser parser;
		parser.setApplicationDescription("ISO 11783-10 Task Controller Server");
		parser.addHelpOption();
		const QCommandLineOption driver("driver", "CAN driver: wcan, pcan_usb, pcan_virtual, virtual or socketcan.", "name");
		const QCommandLineOption channel("channel", "Bus name, CAN-API 2 network name, or SocketCAN interface.", "name");
		const QCommandLineOption tcNumber("tc-number", "TC number, 1..32.", "number");
		const QCommandLineOption booms("booms", "Number of booms the TC reports it supports.", "count");
		const QCommandLineOption sections("sections", "Number of sections the TC reports it supports.", "count");
		const QCommandLineOption channels("channels", "Number of position-based control channels the TC reports it supports.", "count");
		const QCommandLineOption autostart("autostart", "Start the server at once with these settings.");
		parser.addOptions({ driver, channel, tcNumber, booms, sections, channels, autostart });
		parser.process(app);

		QVariantMap options;
		for (const auto &option : { driver, channel })
		{
			if (parser.isSet(option)) options.insert(option.names().first(), parser.value(option));
		}
		for (const auto &option : { tcNumber, booms, sections, channels })
		{
			if (parser.isSet(option)) options.insert(option.names().first(), parser.value(option).toInt());
		}
		options.insert("autostart", parser.isSet(autostart));
		return options;
	}
} // namespace

int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);
	app.setApplicationName("AgIsoTaskControllerServer");
	app.setOrganizationName("Open-Agriculture");
	app.setWindowIcon(QIcon(QStringLiteral(":/icons/logo.ico")));
	const QVariantMap startupOptions = parse_startup_options(app);

	agisotc::TcBridge bridge;

	QQmlApplicationEngine engine;
	engine.rootContext()->setContextProperty("startupOptions", startupOptions);
	engine.rootContext()->setContextProperty("bridge", &bridge);
	engine.rootContext()->setContextProperty("clientModel", bridge.clientModel());
	engine.rootContext()->setContextProperty("ddopModel", bridge.ddopModel());
	engine.rootContext()->setContextProperty("valueModel", bridge.valueModel());
	engine.rootContext()->setContextProperty("ddiTrafficModel", bridge.ddiTrafficModel());
	engine.rootContext()->setContextProperty("logModel", bridge.logModel());

	engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
	if (engine.rootObjects().isEmpty())
	{
		return -1;
	}
	return app.exec();
}
