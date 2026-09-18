//================================================================================================
/// @file TcBridge.hpp
///
/// @brief QObject facade between QML and the TC server core. Owns the server, the CAN
/// setup, and the pump thread. All models are only touched on the GUI thread: a QML
/// timer calls poll() which drains the core's event queue.
//================================================================================================
#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantList>

#include "CanBusManager.hpp"
#include "FieldTaskManager.hpp"
#include "GpsProvider.hpp"
#include "TcModels.hpp"
#include "TcServerCore.hpp"

namespace isobus
{
	class CANMessage;
}

namespace agisotc
{
	class TcBridge : public QObject
	{
		Q_OBJECT
		Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
		Q_PROPERTY(bool taskActive READ isTaskActive NOTIFY taskActiveChanged)
		Q_PROPERTY(int selectedClient READ selectedClient NOTIFY selectedClientChanged)
		Q_PROPERTY(int sectionDdi READ sectionDdi NOTIFY sectionDdiChanged)
		Q_PROPERTY(int sectionCount READ sectionCount NOTIFY sectionCountChanged)
		Q_PROPERTY(QVariantList sectionStates READ sectionStates NOTIFY sectionStatesChanged)
		Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
		Q_PROPERTY(bool gpsRunning READ isGpsRunning NOTIFY gpsChanged)
		Q_PROPERTY(bool gpsValid READ isGpsValid NOTIFY gpsChanged)
		Q_PROPERTY(QString gpsSourceText READ gpsSourceText NOTIFY gpsChanged)
		Q_PROPERTY(double gpsLatitude READ gpsLatitude NOTIFY gpsChanged)
		Q_PROPERTY(double gpsLongitude READ gpsLongitude NOTIFY gpsChanged)
		Q_PROPERTY(double gpsSpeedKph READ gpsSpeedKph NOTIFY gpsChanged)
		Q_PROPERTY(double gpsCourse READ gpsCourse NOTIFY gpsChanged)
		Q_PROPERTY(double tractorX READ tractorX NOTIFY gpsChanged)
		Q_PROPERTY(double tractorZ READ tractorZ NOTIFY gpsChanged)
		Q_PROPERTY(QVariantList trackPoints READ trackPoints NOTIFY trackChanged)
		Q_PROPERTY(QStringList fieldNames READ fieldNames NOTIFY fieldsChanged)
		Q_PROPERTY(QStringList taskNames READ taskNames NOTIFY tasksChanged)
		Q_PROPERTY(int selectedFieldIndex READ selectedFieldIndex NOTIFY fieldsChanged)
		Q_PROPERTY(int selectedTaskIndex READ selectedTaskIndex NOTIFY tasksChanged)
		Q_PROPERTY(QString activeFieldName READ activeFieldName NOTIFY fieldsChanged)
		Q_PROPERTY(QString activeTaskName READ activeTaskName NOTIFY tasksChanged)
		Q_PROPERTY(double fieldWidthM READ fieldWidthM NOTIFY fieldsChanged)
		Q_PROPERTY(double fieldLengthM READ fieldLengthM NOTIFY fieldsChanged)
		Q_PROPERTY(ClientListModel *clientModel READ clientModel CONSTANT)
		Q_PROPERTY(DdopModel *ddopModel READ ddopModel CONSTANT)
		Q_PROPERTY(ProcessDataModel *valueModel READ valueModel CONSTANT)
		Q_PROPERTY(LogModel *logModel READ logModel CONSTANT)

	public:
		/// Measurement trigger kinds, matching the TC process data command bytes.
		enum class MeasurementKind : int
		{
			TimeInterval = 4,
			DistanceInterval = 5,
			MinimumThreshold = 6,
			MaximumThreshold = 7,
			ChangeThreshold = 8
		};
		Q_ENUM(MeasurementKind)

		explicit TcBridge(QObject *parent = nullptr);
		~TcBridge() override;

		bool isRunning() const;
		bool isTaskActive() const;
		int selectedClient() const;
		int sectionDdi() const;
		int sectionCount() const;
		QVariantList sectionStates() const;
		QString statusText() const;
		bool isGpsRunning() const;
		bool isGpsValid() const;
		QString gpsSourceText() const;
		double gpsLatitude() const;
		double gpsLongitude() const;
		double gpsSpeedKph() const;
		double gpsCourse() const;
		double tractorX() const;
		double tractorZ() const;
		QVariantList trackPoints() const;
		QStringList fieldNames() const;
		QStringList taskNames() const;
		int selectedFieldIndex() const;
		int selectedTaskIndex() const;
		QString activeFieldName() const;
		QString activeTaskName() const;
		double fieldWidthM() const;
		double fieldLengthM() const;
		ClientListModel *clientModel();
		DdopModel *ddopModel();
		ProcessDataModel *valueModel();
		LogModel *logModel();

		Q_INVOKABLE bool startServer(const QString &driver, const QString &channel, int tcNumber, int booms, int sections, int channels);
		Q_INVOKABLE void stopServer();
		Q_INVOKABLE void poll();
		Q_INVOKABLE void selectClient(int address);
		Q_INVOKABLE void setTaskActive(bool active);
		Q_INVOKABLE void requestValue(int ddi, int element);
		Q_INVOKABLE void setValue(int ddi, int element, int value, bool acknowledge);
		Q_INVOKABLE void sendMeasurement(int kind, int ddi, int element, int value);
		Q_INVOKABLE void setSectionDdi(int ddi);
		Q_INVOKABLE void setSectionCount(int count);
		Q_INVOKABLE void loadPoolFile(const QUrl &fileUrl);
		Q_INVOKABLE void clearPool();
		Q_INVOKABLE void clearLog();
		Q_INVOKABLE bool startGps(const QString &source, const QString &serialPort, int baudRate,
		                          double latitude, double longitude);
		Q_INVOKABLE void stopGps();
		Q_INVOKABLE void setSimulationMotion(double speedKph, double courseDeg);
		Q_INVOKABLE void nudgeSimulation(double forwardMeters, double turnDegrees);
		Q_INVOKABLE bool createField(const QString &name, double widthM, double lengthM);
		Q_INVOKABLE void selectField(int index);
		Q_INVOKABLE bool createTask(const QString &name);
		Q_INVOKABLE void selectTask(int index);
		Q_INVOKABLE void startSelectedTask();
		Q_INVOKABLE void pauseSelectedTask();
		Q_INVOKABLE void stopSelectedTask();
		Q_INVOKABLE void clearTrack();

	signals:
		void runningChanged();
		void taskActiveChanged();
		void selectedClientChanged();
		void sectionDdiChanged();
		void sectionCountChanged();
		void sectionStatesChanged();
		void statusTextChanged();
		void gpsChanged();
		void trackChanged();
		void fieldsChanged();
		void tasksChanged();
		void identifyBanner(int tcNumber);

	private:
		void setStatus(const QString &text);
		void refreshClients();
		void refreshDdop();
		void pumpLoop();
		void updateGps();
		void updateFieldSelection();
		void refreshFieldNames();
		void refreshTaskNames();
		void registerGpsCanCallbacks();
		void unregisterGpsCanCallbacks();
		static void processGpsCanMessage(const isobus::CANMessage &message, void *parentPointer);

		CanBusManager canBus;
		GpsProvider gpsProvider;
		FieldTaskManager fieldTaskManager;
		std::shared_ptr<GuiTaskControllerServer> server;
		std::thread pumpThread;
		std::atomic_bool pumpRunning = { false };

		ClientListModel clients;
		DdopModel ddop;
		ProcessDataModel values;
		LogModel logs;

		bool running = false;
		bool taskActive = false;
		int currentSelectedClient = -1;
		int currentSectionDdi = 0;
		int currentSectionCount = 16;
		QVariantList currentSectionStates;
		QString currentStatusText = "Server stopped.";
		bool gpsRunningFlag = false;
		bool gpsCanCallbacksRegistered = false;
		QString currentGpsSourceText = "Off";
		GpsSolution currentGps;
		double currentTractorX = 0.0;
		double currentTractorZ = 0.0;
		QVariantList currentTrackPoints;
		std::vector<std::string> fieldIds;
		std::vector<std::string> taskIds;
		QStringList currentFieldNames;
		QStringList currentTaskNames;
		int currentSelectedField = -1;
		int currentSelectedTask = -1;
		QString currentActiveFieldName;
		QString currentActiveTaskName;
		double currentFieldWidthM = 200.0;
		double currentFieldLengthM = 300.0;
		double fieldOriginLatitude = 0.0;
		double fieldOriginLongitude = 0.0;
		bool fieldOriginValid = false;
		std::vector<std::uint8_t> manualPool; ///< Manually loaded pool file for the selected client.
		int manualPoolClient = -1;
	};
} // namespace agisotc
