//================================================================================================
/// @file TcModels.hpp
///
/// @brief QAbstractListModels backing the QML views. Only ever touched on the GUI thread.
//================================================================================================
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
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

	struct DdiTrafficRow
	{
		QString timestamp;
		QString direction;
		QString command;
		int address = -1;
		int ddi = 0;
		int element = 0;
		qint32 value = 0;
		QString detail;
	};

	class DdiTrafficModel : public QAbstractListModel
	{
		Q_OBJECT
	public:
		enum Roles
		{
			TimestampRole = Qt::UserRole + 1,
			DirectionRole,
			CommandRole,
			AddressRole,
			DdiRole,
			ElementRole,
			ValueRole,
			DetailRole
		};

		explicit DdiTrafficModel(QObject *parent = nullptr);

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
		QHash<int, QByteArray> roleNames() const override;

		void addRow(const DdiTrafficRow &row);
		void clear();

	private:
		QList<DdiTrafficRow> rows;
		static constexpr int MAX_ROWS = 1000;
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

	/// @brief Rows of named values that views such as Repeater3D show, updated in place: a
	/// changed row sends dataChanged and new rows are inserted, so views keep their delegates
	/// instead of rebuilding all of them on every update. Delegates read roles as model.<name>.
	class VariantListModel : public QAbstractListModel
	{
		Q_OBJECT
		Q_PROPERTY(int count READ count NOTIFY countChanged)
	public:
		explicit VariantListModel(const QStringList &roles, QObject *parent = nullptr);

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
		QHash<int, QByteArray> roleNames() const override;

		int count() const;
		/// @brief Replaces all rows, telling views only which rows changed, came or went.
		void setRows(const QVariantList &newRows);
		void setRow(int index, const QVariantMap &row);
		void appendRow(const QVariantMap &row);
		void clear();

	signals:
		void countChanged();

	private:
		QStringList keys;
		QList<QVariantMap> rows;
	};
} // namespace agisotc
