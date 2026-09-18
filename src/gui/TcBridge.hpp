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
#include <QUrl>
#include <QVariantList>

#include "CanBusManager.hpp"
#include "TcModels.hpp"
#include "TcServerCore.hpp"

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

	signals:
		void runningChanged();
		void taskActiveChanged();
		void selectedClientChanged();
		void sectionDdiChanged();
		void sectionCountChanged();
		void sectionStatesChanged();
		void statusTextChanged();
		void identifyBanner(int tcNumber);

	private:
		void setStatus(const QString &text);
		void refreshClients();
		void refreshDdop();
		void pumpLoop();

		CanBusManager canBus;
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
		std::vector<std::uint8_t> manualPool; ///< Manually loaded pool file for the selected client.
		int manualPoolClient = -1;
	};
} // namespace agisotc
