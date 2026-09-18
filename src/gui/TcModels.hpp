//================================================================================================
/// @file TcModels.hpp
///
/// @brief QAbstractListModels backing the QML views. Only ever touched on the GUI thread.
//================================================================================================
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QVector>

namespace agisotc
{
	struct ClientRow
	{
		int address = -1;
		QString nameHex;
		int functionCode = 0;
		int functionInstance = 0;
		int manufacturerCode = 0;
		quint32 identityNumber = 0;
		quint32 ddopSizeBytes = 0;
		bool ddopActive = false;
		bool timedOut = false;
		int reportedVersion = 0;
		quint32 statusBits = 0;
		QString lastSeen;
	};

	class ClientListModel : public QAbstractListModel
	{
		Q_OBJECT
	public:
		enum Roles
		{
			AddressRole = Qt::UserRole + 1,
			NameRole,
			FunctionRole,
			ManufacturerRole,
			IdentityRole,
			DdopSizeRole,
			DdopActiveRole,
			TimedOutRole,
			VersionRole,
			StatusRole,
			LastSeenRole
		};

		explicit ClientListModel(QObject *parent = nullptr);

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
		QHash<int, QByteArray> roleNames() const override;

		void setClients(const QList<ClientRow> &clients);

	private:
		QList<ClientRow> rows;
	};

	struct DdopRow
	{
		int indent = 0;
		QString text;
	};

	class DdopModel : public QAbstractListModel
	{
		Q_OBJECT
	public:
		enum Roles
		{
			IndentRole = Qt::UserRole + 1,
			TextRole
		};

		explicit DdopModel(QObject *parent = nullptr);

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
		QHash<int, QByteArray> roleNames() const override;

		void setRows(const QList<DdopRow> &rows);

	private:
		QList<DdopRow> rows;
	};

	struct ValueRow
	{
		int address = -1;
		int ddi = 0;
		int element = 0;
		qint32 value = 0;
		QString timestamp;
	};

	class ProcessDataModel : public QAbstractListModel
	{
		Q_OBJECT
	public:
		enum Roles
		{
			AddressRole = Qt::UserRole + 1,
			DdiRole,
			ElementRole,
			ValueRole,
			TimestampRole
		};

		explicit ProcessDataModel(QObject *parent = nullptr);

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
		QHash<int, QByteArray> roleNames() const override;

		void upsertValue(int address, int ddi, int element, qint32 value, const QString &timestamp);
		void clear();

	private:
		QList<ValueRow> rows;
		static constexpr int MAX_ROWS = 400;
	};

	class LogModel : public QAbstractListModel
	{
		Q_OBJECT
	public:
		enum Roles
		{
			LineRole = Qt::UserRole + 1
		};

		explicit LogModel(QObject *parent = nullptr);

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
		QHash<int, QByteArray> roleNames() const override;

		void addLine(const QString &line);
		void clear();

	private:
		QList<QString> lines;
		static constexpr int MAX_LINES = 1000;
	};
} // namespace agisotc
