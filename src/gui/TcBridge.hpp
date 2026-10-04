//================================================================================================
/// @file TcBridge.hpp
///
/// @brief QObject facade between QML and the TC server core. Owns the server, the CAN
/// setup, and the pump thread. All models are only touched on the GUI thread: a QML
/// timer calls poll() which drains the core's event queue.
//================================================================================================
#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <thread>
#include <tuple>
#include <vector>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

#include "CanBusManager.hpp"
#include "ConnectionProgress.hpp"
#include "CoverageMap.hpp"
#include "FieldTaskManager.hpp"
#include "GpsProvider.hpp"
#include "PrescriptionImage.hpp"
#include "PrescriptionMap.hpp"
#include "RatePlan.hpp"
#include "SectionController.hpp"
#include "SectionPlanner.hpp"
#include "StackLog.hpp"
#include "TcClientPlan.hpp"
#include "TcModels.hpp"
#include "TcServerCore.hpp"

namespace isobus
{
	class CANMessage;
	class DeviceDescriptorObjectPool;
	class NMEA2000MessageInterface;
	class SpeedMessagesInterface;
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
		Q_PROPERTY(QVariantList coveragePatches READ coveragePatches NOTIFY workChanged)
		Q_PROPERTY(VariantListModel *coveragePatchModel READ coveragePatchModel CONSTANT)
		Q_PROPERTY(VariantListModel *implementElementModel READ implementElementModel CONSTANT)
		/// The booms that carry sections, each with its sections' place, width and state: the LED
		/// bars of the TC-SC views. Rows: index, element, name, count, onCount, left, right, widthM
		/// (metres across the implement, right of the connector), z (metres behind the connector,
		/// where the 3D view draws the bar), and sections (number, element, name, left, width, on).
		Q_PROPERTY(QVariantList booms READ booms NOTIFY implementChanged)
		/// The same as one LED per row, plus one "rail" row per boom, for the 3D view: kind, boom,
		/// number, x (right), z (rearward of the connector), width, on. Updated in place.
		Q_PROPERTY(VariantListModel *boomLedModel READ boomLedModel CONSTANT)
		Q_PROPERTY(bool autoSectionControl READ autoSectionControl NOTIFY sectionControlChanged)
		Q_PROPERTY(QString sectionControlStatus READ sectionControlStatus NOTIFY sectionControlChanged)
		/// TC-GEO position-based control of the selected client, per control channel (a device
		/// element with a Prescription Control State): index, element, name, hasState, and groups,
		/// one per DDI and bin, each with its source (0 off, 1 the fixed rate, 2 + n layer n of the
		/// prescription), its channel-level target (top) and the sub-boom or section targets of a
		/// multi-rate device (subs). Changes only with the plan or a choice; values: rateLive.
		Q_PROPERTY(QVariantList rateChannels READ rateChannels NOTIFY rateControlChanged)
		/// The live values, refreshed four times a second: "c<channel>" engaged, state, latencyMs,
		/// practice; "t<target>" commanded, wanted, actual, source.
		Q_PROPERTY(QVariantMap rateLive READ rateLive NOTIFY rateLiveChanged)
		Q_PROPERTY(QString rateControlStatus READ rateControlStatus NOTIFY rateControlChanged)
		/// The selected task's prescription: present, name, description, layers, selectedLayer,
		/// legend, and its image for the views (imageUrl, centreX, centreZ, width, height in local
		/// metres).
		Q_PROPERTY(QVariantMap prescription READ prescription NOTIFY prescriptionChanged)
		/// One row per rate target, where the TC looks the rate up in the map (ahead of the
		/// element by its setpoint latency): x, z, width, colour, label.
		Q_PROPERTY(VariantListModel *rateMarkerModel READ rateMarkerModel CONSTANT)
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
		/// The implement furthest along in connecting, for a progress display: active, step (1 starting
		/// up, 2 connecting, 3 uploading its DDOP, 4 building, 5 ready), progress (0..1), address,
		/// transferBytes, receivedBytes, uploadSkipped, geometryReceived, geometryTotal, elapsedMs, name.
		Q_PROPERTY(QVariantMap implementLoading READ implementLoading NOTIFY implementLoadingChanged)
		/// True while the selected client is connected with an active pool and built: the views show
		/// the implement only then.
		Q_PROPERTY(bool implementReady READ implementReady NOTIFY implementReadyChanged)
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
	Q_PROPERTY(QVariantList busPeers READ busPeers NOTIFY busPeersChanged)
		Q_PROPERTY(LogModel *logModel READ logModel CONSTANT)

	public:
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
		QVariantList coveragePatches() const;
		VariantListModel *coveragePatchModel();
		VariantListModel *implementElementModel();
		VariantListModel *boomLedModel();
		bool autoSectionControl() const;
		QString sectionControlStatus() const;
		QVariantList rateChannels() const;
		QVariantMap rateLive() const;
		QString rateControlStatus() const;
		QVariantMap prescription() const;
		VariantListModel *rateMarkerModel();
		/// @brief Where the prescription image goes, for the QML image provider.
		std::shared_ptr<PrescriptionImageSlot> prescriptionImageSlot() const;
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
		QVariantMap implementLoading() const;
		bool implementReady() const;
		QString implementName() const;
		QString implementGeometryStatus() const;
		QVariantList implementElements() const;
		QVariantList booms() const;
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
	QVariantList busPeers() const;
		LogModel *logModel();

		Q_INVOKABLE bool startServer(const QString &driver, const QString &channel, int tcNumber, int booms, int sections, int channels);
		Q_INVOKABLE void stopServer();
		Q_INVOKABLE void poll();
		Q_INVOKABLE void selectClient(int address);
		Q_INVOKABLE void setTaskActive(bool active);
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
		Q_INVOKABLE void saveFields(const QUrl &fileUrl);
		Q_INVOKABLE void loadFields(const QUrl &fileUrl);
		Q_INVOKABLE void saveTasks(const QUrl &fileUrl);
		Q_INVOKABLE void loadTasks(const QUrl &fileUrl);
		Q_INVOKABLE bool createFieldFromLocalBoundary(const QString &name, const QVariantList &points);
		Q_INVOKABLE void setSteeringAngle(double degrees);
		Q_INVOKABLE void setThrottleKph(double speedKph);
		Q_INVOKABLE void adjustThrottle(double deltaKph);
		Q_INVOKABLE void stopTractor();
		Q_INVOKABLE void clearWorkedArea();
		/// @brief Lets the TC switch the client's sections while a task is active (TC-SC).
		Q_INVOKABLE void setAutoSectionControl(bool enabled);
		/// @brief Where a rate group's setpoint comes from while a task is active: 0 nowhere (off),
		/// 1 its fixed rate, 2 + n layer n of the task's prescription (variable rate).
		Q_INVOKABLE void setRateGroupSource(int group, int source);
		/// @brief The fixed rate of a rate group, in the DDOP's raw unit.
		Q_INVOKABLE void setRateGroupFixed(int group, int value);
		/// @brief Imports the tasks of an ISO 11783-10 TASKDATA.XML with their prescriptions, and
		/// the partfields they are on as fields.
		Q_INVOKABLE void importTaskData(const QUrl &fileUrl);
		/// @brief Adds a generated layer to the selected task's prescription, a grid over its field.
		/// @param[in] pattern 0 checkerboard, 1 stripes (across the implement), 2 bands (along the
		/// driving direction), 3 gradient from west to east.
		Q_INVOKABLE bool createTestPrescription(int ddi, int pattern, int rateA, int rateB, double cellSizeM, double patternSizeM);
		/// @brief Adds a treatment zone drawn on the map (local metres) to the selected task's prescription.
		Q_INVOKABLE bool addRateZone(const QVariantList &points, int ddi, int value);
		Q_INVOKABLE void clearPrescription();
		/// @brief The layer the views show.
		Q_INVOKABLE void selectPrescriptionLayer(int index);

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
		void busPeersChanged();
		void workChanged();
		void sectionControlChanged();
		void drivingControlsChanged();
		void implementLoadingChanged();
		void implementReadyChanged();
		void rateControlChanged();
		void rateLiveChanged();
		void prescriptionChanged();
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
		/// @brief Rebuilds the booms and the LED bar rows from the plan and the section states.
		void publishBoomLeds(const std::array<double, 2> &connectorOffset);
		void updateImplementValue(std::uint16_t ddi, std::uint16_t element, std::int32_t value);
		void serviceDdiSync();
		void appendDdiTraffic(const QString &direction, const QString &command, int address, int ddi, int element,
		                      std::int32_t value, const QString &detail);
		void updateTrailerPose(double elapsedSeconds);
		void updateWorkCoverage(double elapsedSeconds);
		void updateSpeedMessages(double elapsedSeconds);
		void updateNmea2000Gps();
		void rebuildFieldBoundaryPoints();
		void refreshBusPeers();
		/// @brief Moves the CAN stack's log lines into the event log.
		void drainStackLog();
		/// @brief True while the client is connected with an active pool (and the server runs).
		bool isClientOnline(int address);
		/// @brief Follows the connecting implements: geometry of the selected client, the progress
		/// for the views, the log line of an implement that got ready, and implementReady.
		void serviceImplementLoading(std::uint64_t nowMs);
		/// @brief Asks the selected client again for the geometry values still missing while its
		/// implement is being built, so one lost request does not hold the implement back.
		void serviceGeometryRequests(std::uint64_t nowMs);
		void ingestSniffedProcessData(int source, int destination, bool outgoing,
		                              std::uint8_t command, std::uint16_t ddi,
		                              std::uint16_t element, std::int32_t value);
		static void processGpsCanMessage(const isobus::CANMessage &message, void *parentPointer);
		void drainBusFrames();

		// --- TC controller (TC-BAS set-up, TC-SC, rate control) for the selected client ---

		struct ImplementElementState;

		/// @brief A section on the ground now: its centre and width.
		struct SectionPose
		{
			GroundPoint centre;
			double widthM = 0.0;
		};

		/// @brief Builds the plan from the selected client's active pool and sends its set-up.
		void setupClientPlan();
		/// @brief Sends the plan's set-up again (at activation, and at every task start, since
		/// a client may drop its measurements when the task stops).
		void sendSetupCommands();
		void sendTcCommands(const std::vector<TcCommand> &commands);
		/// @brief Engages section control while a task is active and sends the wanted states.
		void serviceSectionControl(std::uint64_t nowMs);
		std::vector<SectionPose> sectionPoses() const;
		std::vector<bool> wantedSectionStates(const std::vector<SectionPose> &poses, std::uint64_t nowMs) const;
		/// @brief Rebuilds the implement lists for the views; poll() does it at most every
		/// IMPLEMENT_PUBLISH_MS, however many values arrive in between.
		void publishImplementModelIfDue(std::uint64_t nowMs);
		/// @brief True for the elements the 3D view shows: the connector, and the booms that
		/// carry sections, with those sections. A function without sections is no boom.
		bool isShownElement(const ImplementElementState &element) const;
		/// @brief Actual on/off of each plan section: as the client reports it, else as commanded.
		std::vector<bool> appliedSectionStates() const;
		/// @brief Offset of an element from the device reference point in metres (ISO axes: X
		/// forward, Y right). A missing offset is taken from the nearest element above it.
		std::array<double, 2> elementOffset(const ImplementElementState &element) const;
		void extendCoveragePatch(std::size_t section, GroundPoint from, GroundPoint to, double widthM);
		void publishCoverage(bool force);
		/// @brief The connector's offset from the device reference point: the implement's hitch.
		std::array<double, 2> connectorOffset() const;
		/// @brief Where an element offset (ISO axes, metres) lies on the ground now.
		GroundPoint implementGround(const std::array<double, 2> &offset, const std::array<double, 2> &connector) const;

		// --- TC-GEO: variable rate and multi-rate control (TcBridgeRateControl.cpp) ---

		/// @brief Builds the rate plan from the selected client's pool and the DDIs of the
		/// selected task's prescription.
		/// @param[in] keepWhenSame Keeps the engaged channels when the plan stays the same (a new
		/// prescription); otherwise they are released first (the client activated its pool again).
		void setupRatePlan(isobus::DeviceDescriptorObjectPool *pool, bool keepWhenSame);
		/// @brief Engages the channels whose groups have a source while a task is active, and
		/// sends each target its rate: the fixed one, or the map's at its position.
		void serviceRateControl(std::uint64_t nowMs);
		/// @brief The source of a group now: 0 off, 1 fixed, 2 + n layer n.
		int rateGroupSource(std::size_t group) const;
		/// @brief The prescription layer for a group: same DDI, practice and instance.
		std::optional<std::size_t> matchedLayer(std::size_t group) const;
		std::optional<std::int32_t> receivedValue(std::uint16_t element, std::uint16_t ddi) const;
		void publishRateChannels(std::uint64_t nowMs, bool force);
		const Prescription *selectedPrescription() const;
		/// @brief The selected task's prescription, made when it has none; nullptr without a task.
		Prescription *editablePrescription();
		/// @brief After the selected task or its prescription changed: the views and the rate plan.
		void refreshPrescription(bool rebuildPlan);
		void renderPrescription();
		GeoPoint localToGeo(GroundPoint point) const;
		GroundPoint geoToLocal(GeoPoint point) const;

		CanBusManager canBus;
		GpsProvider gpsProvider;
		FieldTaskManager fieldTaskManager;
		std::shared_ptr<GuiTaskControllerServer> server;
		std::unique_ptr<isobus::SpeedMessagesInterface> speedMessages;
		std::unique_ptr<isobus::NMEA2000MessageInterface> nmea2000;
		std::thread pumpThread;
		std::atomic_bool pumpRunning = { false };

		ClientListModel clients;
		DdopModel ddop;
		ProcessDataModel values;
		DdiTrafficModel ddiTraffic;
		LogModel logs;

		struct BusPeerInfo
		{
			int functionCode = 0;
			int functionInstance = 0;
			int manufacturerCode = 0;
			std::uint64_t lastSeenMs = 0;
		};
		std::map<std::uint8_t, BusPeerInfo> busPeersByAddress;
		QVariantList currentBusPeers;
		std::set<std::uint8_t> connectedAddresses;

		StackLog stackLog;
		bool stackLogVerbose = false;

		// Implements connecting, from the bus to a built implement.
		ConnectionProgress connectionProgress;
		/// Per client address: pool active and timed out as last seen, to notice activations and time-outs.
		struct ClientLink
		{
			bool active = false;
			bool timedOut = false;
		};
		std::map<int, ClientLink> clientLinks;
		QVariantMap currentImplementLoading;
		bool selectedClientOnline = false;
		bool implementReadyFlag = false;
		std::uint64_t lastGeometryRequestMs = 0;
		static constexpr std::uint64_t GEOMETRY_REQUEST_MS = 500;

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
			bool hasOffsetX = false; ///< The DDOP gives this element an X offset of its own.
			bool hasOffsetY = false; ///< The DDOP gives this element a Y offset of its own.
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
		bool liveDdiTrafficWatchEnabled = true;
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
		std::uint32_t currentMachineDistanceMm = 0;
		std::uint64_t lastMotionUpdateMs = 0;
		bool trailerPoseValid = false;
		double lastCoverageX = 0.0;
		double lastCoverageZ = 0.0;
		bool coveragePositionValid = false;
		std::vector<std::uint8_t> manualPool; ///< Manually loaded pool file for the selected client.
		int manualPoolClient = -1;

		// TC controller state, for the client in planClient.
		SectionController sectionController;
		int planClient = -1;
		std::uint64_t pendingSetupAtMs = 0; ///< When to send the set-up again, 0 if not due.
		int supportedBooms = 0; ///< What this TC reports it supports, from startServer().
		int supportedSections = 0;
		int supportedChannels = 0;
		bool autoSectionControlEnabled = true;
		QString currentSectionControlStatus = "No client";

		// TC-GEO rate control state, for the client in planClient.
		RateController rateController;
		struct RateGroupChoice
		{
			int source = -1; ///< -1 automatic: the matching prescription layer, else off.
			int fixedValue = 0;
		};
		/// By channel element, DDI and bin (-1 without), so a choice outlives a reconnect.
		std::map<std::tuple<std::uint16_t, std::uint16_t, int>, RateGroupChoice> rateGroupChoices;
		std::vector<std::optional<std::int32_t>> rateWanted; ///< Per target, from its source now.
		std::vector<PrescriptionSource> rateWantedSource;
		std::vector<GroundPoint> rateTargetPoints; ///< Where each target looks the map up.
		std::vector<GroundPoint> rateTargetLast;
		std::vector<GroundPoint> rateTargetVelocity;
		std::uint64_t lastRateMotionMs = 0;
		QVariantList currentRateChannels;
		QVariantMap currentRateLive;
		QString currentRateControlStatus = "No client";
		std::uint64_t lastRatePublishMs = 0;
		std::map<std::string, Prescription> taskPrescriptions; ///< By task id.
		int currentPrescriptionLayer = 0;
		QVariantMap currentPrescription;
		int prescriptionImageSerial = 0;
		std::shared_ptr<PrescriptionImageSlot> prescriptionImages = std::make_shared<PrescriptionImageSlot>();
		VariantListModel rateMarkerRows{ QStringList{ "x", "z", "width", "colour", "label" } };

		// Coverage: a grid for section control and area, and patches for the map views.
		struct CoveragePatch
		{
			GroundPoint start;
			GroundPoint end;
			double widthM = 0.0;
			double courseDeg = 0.0;
		};
		CoverageMap coverage;
		std::vector<CoveragePatch> coveragePatchList;
		std::vector<int> openPatchBySection; ///< Patch a section is extending, -1 if none.
		std::vector<GroundPoint> lastSectionCentres;
		std::vector<GroundPoint> sectionVelocities; ///< How each section moved lately, m/s.
		bool sectionCentresValid = false;
		QVariantList currentCoveragePatches;
		VariantListModel coveragePatchRows{ QStringList{ "x", "z", "length", "width", "course" } };
		std::size_t publishedPatchCount = 0; ///< Patches the views already have
		std::set<std::size_t> changedPatches; ///< Published patches extended since
		std::uint64_t lastCoveragePublishMs = 0;

		static constexpr std::uint64_t IMPLEMENT_PUBLISH_MS = 250;
		VariantListModel implementElementRows{ QStringList{ "objectId", "element", "name", "type", "x", "y", "z",
			                                                "width", "length", "height", "active", "hasGeometry" } };
		bool implementModelDirty = false;
		std::uint64_t lastImplementPublishMs = 0;
		QVariantList currentBooms;
		VariantListModel boomLedRows{ QStringList{ "kind", "boom", "number", "x", "z", "width", "on" } };
		std::vector<bool> publishedLedStates; ///< Section states the LED bars show, in plan order.
		std::vector<GroundPoint> boundaryLocal; ///< Selected field's boundary in local metres.
	};
} // namespace agisotc
