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
#include <vector>

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
	class DeviceDescriptorObjectPool;
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
		Q_PROPERTY(QVariantList workedPoints READ workedPoints NOTIFY workChanged)
		Q_PROPERTY(QVariantList fieldBoundaryPoints READ fieldBoundaryPoints NOTIFY boundaryChanged)
		Q_PROPERTY(bool boundaryRecording READ boundaryRecording NOTIFY boundaryChanged)
		Q_PROPERTY(int boundaryPointCount READ boundaryPointCount NOTIFY boundaryChanged)
		Q_PROPERTY(QStringList fieldNames READ fieldNames NOTIFY fieldsChanged)
		Q_PROPERTY(QStringList taskNames READ taskNames NOTIFY tasksChanged)
		Q_PROPERTY(int selectedFieldIndex READ selectedFieldIndex NOTIFY fieldsChanged)
		Q_PROPERTY(int selectedTaskIndex READ selectedTaskIndex NOTIFY tasksChanged)
		Q_PROPERTY(QString activeFieldName READ activeFieldName NOTIFY fieldsChanged)
		Q_PROPERTY(QString activeTaskName READ activeTaskName NOTIFY tasksChanged)
		Q_PROPERTY(double fieldWidthM READ fieldWidthM NOTIFY fieldsChanged)
		Q_PROPERTY(double fieldLengthM READ fieldLengthM NOTIFY fieldsChanged)
		Q_PROPERTY(QString implementName READ implementName NOTIFY implementChanged)
		Q_PROPERTY(QString implementGeometryStatus READ implementGeometryStatus NOTIFY implementChanged)
		Q_PROPERTY(QVariantList implementElements READ implementElements NOTIFY implementChanged)
		Q_PROPERTY(QVariantList implementDdis READ implementDdis NOTIFY implementDdisChanged)
		Q_PROPERTY(bool autoDdiSync READ autoDdiSync NOTIFY autoDdiSyncChanged)
		Q_PROPERTY(int ddiSyncIntervalMs READ ddiSyncIntervalMs NOTIFY autoDdiSyncChanged)
		Q_PROPERTY(bool liveDdiTrafficWatch READ liveDdiTrafficWatch NOTIFY liveDdiTrafficWatchChanged)
		Q_PROPERTY(QVariantList tcBasicData READ tcBasicData NOTIFY implementDdisChanged)
		Q_PROPERTY(int activeSectionCount READ activeSectionCount NOTIFY sectionStatesChanged)
		Q_PROPERTY(double workedAreaHa READ workedAreaHa NOTIFY workChanged)
		Q_PROPERTY(double workedDistanceM READ workedDistanceM NOTIFY workChanged)
		Q_PROPERTY(double workedTimeSeconds READ workedTimeSeconds NOTIFY workChanged)
		Q_PROPERTY(double steeringAngle READ steeringAngle NOTIFY drivingControlsChanged)
		Q_PROPERTY(double throttleKph READ throttleKph NOTIFY drivingControlsChanged)
		Q_PROPERTY(double implementX READ implementX NOTIFY gpsChanged)
		Q_PROPERTY(double implementZ READ implementZ NOTIFY gpsChanged)
		Q_PROPERTY(double implementCourse READ implementCourse NOTIFY gpsChanged)
		Q_PROPERTY(ClientListModel *clientModel READ clientModel CONSTANT)
		Q_PROPERTY(DdopModel *ddopModel READ ddopModel CONSTANT)
		Q_PROPERTY(ProcessDataModel *valueModel READ valueModel CONSTANT)
		Q_PROPERTY(DdiTrafficModel *ddiTrafficModel READ ddiTrafficModel CONSTANT)
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
		QVariantList workedPoints() const;
		QVariantList fieldBoundaryPoints() const;
		bool boundaryRecording() const;
		int boundaryPointCount() const;
		QStringList fieldNames() const;
		QStringList taskNames() const;
		int selectedFieldIndex() const;
		int selectedTaskIndex() const;
		QString activeFieldName() const;
		QString activeTaskName() const;
		double fieldWidthM() const;
		double fieldLengthM() const;
		QString implementName() const;
		QString implementGeometryStatus() const;
		QVariantList implementElements() const;
		QVariantList implementDdis() const;
		bool autoDdiSync() const;
		int ddiSyncIntervalMs() const;
		bool liveDdiTrafficWatch() const;
		QVariantList tcBasicData() const;
		int activeSectionCount() const;
		double workedAreaHa() const;
		double workedDistanceM() const;
		double workedTimeSeconds() const;
		double steeringAngle() const;
		double throttleKph() const;
		double implementX() const;
		double implementZ() const;
		double implementCourse() const;
		ClientListModel *clientModel();
		DdopModel *ddopModel();
		ProcessDataModel *valueModel();
		DdiTrafficModel *ddiTrafficModel();
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
		Q_INVOKABLE void setAutoDdiSync(bool enabled);
		Q_INVOKABLE void setDdiSyncIntervalMs(int intervalMs);
		Q_INVOKABLE void setLiveDdiTrafficWatch(bool enabled);
		Q_INVOKABLE void clearDdiTraffic();
		Q_INVOKABLE void requestImplementDdis();
		Q_INVOKABLE bool startBoundaryRecording(const QString &name);
		Q_INVOKABLE bool finishBoundaryRecording();
		Q_INVOKABLE void cancelBoundaryRecording();
		Q_INVOKABLE bool createFieldFromLocalBoundary(const QString &name, const QVariantList &points);
		Q_INVOKABLE void setSteeringAngle(double degrees);
		Q_INVOKABLE void setThrottleKph(double speedKph);
		Q_INVOKABLE void adjustThrottle(double deltaKph);
		Q_INVOKABLE void stopTractor();
		Q_INVOKABLE void clearWorkedArea();

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
		void implementChanged();
		void implementDdisChanged();
		void autoDdiSyncChanged();
		void liveDdiTrafficWatchChanged();
		void boundaryChanged();
		void workChanged();
		void drivingControlsChanged();
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
		void clearImplementModel();
		void buildImplementModel(isobus::DeviceDescriptorObjectPool &pool);
		void publishImplementModel();
		void updateImplementValue(std::uint16_t ddi, std::uint16_t element, std::int32_t value);
		void serviceDdiSync();
		void appendDdiTraffic(const QString &direction, const QString &command, int address, int ddi, int element,
		                      std::int32_t value, const QString &detail);
		void updateTrailerPose(double elapsedSeconds);
		void updateWorkCoverage(double elapsedSeconds);
		void rebuildFieldBoundaryPoints();
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
		DdiTrafficModel ddiTraffic;
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
		QVariantList currentWorkedPoints;
		QVariantList currentFieldBoundaryPoints;
		std::vector<std::pair<double, double>> recordedBoundary;
		QString recordedBoundaryName;
		bool boundaryRecordingFlag = false;
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

		struct ImplementElementState
		{
			std::uint16_t objectId = 0;
			std::uint16_t element = 0;
			std::uint16_t parentObjectId = 0;
			int type = 0;
			QString name;
			double localX = 0.0;
			double localY = 0.0;
			double localZ = 0.0;
			double width = 0.0;
			double length = 0.0;
			double height = 0.0;
			bool hasGeometry = false;
			bool active = false;
		};

		struct ImplementDdiState
		{
			std::uint16_t ddi = 0;
			std::uint16_t element = 0;
			std::uint8_t triggers = 0;
			bool settable = false;
			QString name;
			bool hasValue = false;
			std::int32_t value = 0;
			QString updated;
			int geometryKind = 0; ///< 1/2/3 offset XYZ, 4/5/6 width/length/height.
			double geometryOffset = 0.0;
			double geometryScale = 0.001;
			double displayOffset = 0.0;
			double displayScale = 1.0;
			QString unit;
			bool reportingConfigured = false;
		};

		QString currentImplementName = "No implement DDOP";
		QString currentImplementGeometryStatus = "Waiting for an implement object pool";
		QVariantList currentImplementElements;
		QVariantList currentImplementDdis;
		std::vector<ImplementElementState> implementElementStates;
		std::vector<ImplementDdiState> implementDdiStates;
		bool autoDdiSyncEnabled = true;
		int currentDdiSyncIntervalMs = 1000;
		bool liveDdiTrafficWatchEnabled = false;
		std::uint64_t lastDdiSyncMs = 0;
		std::size_t nextDdiSyncIndex = 0;
		QVariantList currentTcBasicData;
		double currentWorkedAreaHa = 0.0;
		double currentWorkedDistanceM = 0.0;
		double currentWorkedTimeSeconds = 0.0;
		double currentSteeringAngle = 0.0;
		double currentThrottleKph = 0.0;
		double currentImplementX = 0.0;
		double currentImplementZ = 0.0;
		double currentImplementCourse = 0.0;
		std::uint64_t lastMotionUpdateMs = 0;
		bool trailerPoseValid = false;
		double lastCoverageX = 0.0;
		double lastCoverageZ = 0.0;
		bool coveragePositionValid = false;
		std::vector<std::uint8_t> manualPool; ///< Manually loaded pool file for the selected client.
		int manualPoolClient = -1;
	};
} // namespace agisotc
