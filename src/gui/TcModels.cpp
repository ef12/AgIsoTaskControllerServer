#include "TcModels.hpp"

namespace agisotc
{
	ClientListModel::ClientListModel(QObject *parent) :
	  QAbstractListModel(parent)
	{
	}

	int ClientListModel::rowCount(const QModelIndex &parent) const
	{
		return parent.isValid() ? 0 : rows.size();
	}

	QVariant ClientListModel::data(const QModelIndex &index, int role) const
	{
		if (!index.isValid() || index.row() < 0 || index.row() >= rows.size())
		{
			return {};
		}
		const auto &row = rows.at(index.row());
		switch (role)
		{
			case AddressRole:
				return row.address;
			case NameRole:
				return row.nameHex;
			case FunctionRole:
				return QString("%1/%2").arg(row.functionCode).arg(row.functionInstance);
			case ManufacturerRole:
				return row.manufacturerCode;
			case IdentityRole:
				return row.identityNumber;
			case DdopSizeRole:
				return row.ddopSizeBytes;
			case DdopActiveRole:
				return row.ddopActive;
			case TimedOutRole:
				return row.timedOut;
			case VersionRole:
				return row.reportedVersion;
			case StatusRole:
				return row.statusBits;
			case LastSeenRole:
				return row.lastSeen;
			default:
				return {};
		}
	}

	QHash<int, QByteArray> ClientListModel::roleNames() const
	{
		return {
			{ AddressRole, "clientAddress" },
			{ NameRole, "clientName" },
			{ FunctionRole, "clientFunction" },
			{ ManufacturerRole, "manufacturerCode" },
			{ IdentityRole, "identityNumber" },
			{ DdopSizeRole, "ddopSize" },
			{ DdopActiveRole, "ddopActive" },
			{ TimedOutRole, "timedOut" },
			{ VersionRole, "reportedVersion" },
			{ StatusRole, "statusBits" },
			{ LastSeenRole, "lastSeen" },
		};
	}

	void ClientListModel::setClients(const QList<ClientRow> &clients)
	{
		beginResetModel();
		rows = clients;
		endResetModel();
	}

	DdopModel::DdopModel(QObject *parent) :
	  QAbstractListModel(parent)
	{
	}

	int DdopModel::rowCount(const QModelIndex &parent) const
	{
		return parent.isValid() ? 0 : rows.size();
	}

	QVariant DdopModel::data(const QModelIndex &index, int role) const
	{
		if (!index.isValid() || index.row() < 0 || index.row() >= rows.size())
		{
			return {};
		}
		const auto &row = rows.at(index.row());
		if (IndentRole == role)
		{
			return row.indent;
		}
		if (TextRole == role)
		{
			return row.text;
		}
		return {};
	}

	QHash<int, QByteArray> DdopModel::roleNames() const
	{
		return {
			{ IndentRole, "indent" },
			{ TextRole, "rowText" },
		};
	}

	void DdopModel::setRows(const QList<DdopRow> &newRows)
	{
		beginResetModel();
		rows = newRows;
		endResetModel();
	}

	ProcessDataModel::ProcessDataModel(QObject *parent) :
	  QAbstractListModel(parent)
	{
	}

	int ProcessDataModel::rowCount(const QModelIndex &parent) const
	{
		return parent.isValid() ? 0 : rows.size();
	}

	QVariant ProcessDataModel::data(const QModelIndex &index, int role) const
	{
		if (!index.isValid() || index.row() < 0 || index.row() >= rows.size())
		{
			return {};
		}
		const auto &row = rows.at(index.row());
		switch (role)
		{
			case AddressRole:
				return row.address;
			case DdiRole:
				return row.ddi;
			case ElementRole:
				return row.element;
			case ValueRole:
				return row.value;
			case TimestampRole:
				return row.timestamp;
			default:
				return {};
		}
	}

	QHash<int, QByteArray> ProcessDataModel::roleNames() const
	{
		return {
			{ AddressRole, "valueAddress" },
			{ DdiRole, "valueDdi" },
			{ ElementRole, "valueElement" },
			{ ValueRole, "valueContent" },
			{ TimestampRole, "valueTime" },
		};
	}

	void ProcessDataModel::upsertValue(int address, int ddi, int element, qint32 value, const QString &timestamp)
	{
		for (int i = 0; i < rows.size(); ++i)
		{
			if ((rows[i].address == address) && (rows[i].ddi == ddi) && (rows[i].element == element))
			{
				rows[i].value = value;
				rows[i].timestamp = timestamp;
				const auto modelIndex = index(i, 0);
				emit dataChanged(modelIndex, modelIndex, { ValueRole, TimestampRole });
				return;
			}
		}
		if (rows.size() >= MAX_ROWS)
		{
			beginRemoveRows(QModelIndex(), 0, 0);
			rows.removeFirst();
			endRemoveRows();
		}
		beginInsertRows(QModelIndex(), rows.size(), rows.size());
		rows.push_back({ address, ddi, element, value, timestamp });
		endInsertRows();
	}

	void ProcessDataModel::clear()
	{
		beginResetModel();
		rows.clear();
		endResetModel();
	}

	DdiTrafficModel::DdiTrafficModel(QObject *parent) :
	  QAbstractListModel(parent)
	{
	}

	int DdiTrafficModel::rowCount(const QModelIndex &parent) const
	{
		return parent.isValid() ? 0 : rows.size();
	}

	QVariant DdiTrafficModel::data(const QModelIndex &index, int role) const
	{
		if (!index.isValid() || index.row() < 0 || index.row() >= rows.size())
		{
			return {};
		}
		const auto &row = rows.at(index.row());
		switch (role)
		{
			case TimestampRole:
				return row.timestamp;
			case DirectionRole:
				return row.direction;
			case CommandRole:
				return row.command;
			case AddressRole:
				return row.address;
			case DdiRole:
				return row.ddi;
			case ElementRole:
				return row.element;
			case ValueRole:
				return row.value;
			case DetailRole:
				return row.detail;
			default:
				return {};
		}
	}

	QHash<int, QByteArray> DdiTrafficModel::roleNames() const
	{
		return {
			{ TimestampRole, "trafficTime" },
			{ DirectionRole, "trafficDirection" },
			{ CommandRole, "trafficCommand" },
			{ AddressRole, "trafficAddress" },
			{ DdiRole, "trafficDdi" },
			{ ElementRole, "trafficElement" },
			{ ValueRole, "trafficValue" },
			{ DetailRole, "trafficDetail" },
		};
	}

	void DdiTrafficModel::addRow(const DdiTrafficRow &row)
	{
		if (rows.size() >= MAX_ROWS)
		{
			beginRemoveRows(QModelIndex(), 0, 0);
			rows.removeFirst();
			endRemoveRows();
		}
		beginInsertRows(QModelIndex(), rows.size(), rows.size());
		rows.push_back(row);
		endInsertRows();
	}

	void DdiTrafficModel::clear()
	{
		beginResetModel();
		rows.clear();
		endResetModel();
	}

	BusMonitorModel::BusMonitorModel(QObject *parent) :
	  QAbstractListModel(parent)
	{
	}

	int BusMonitorModel::rowCount(const QModelIndex &parent) const
	{
		return parent.isValid() ? 0 : rows.size();
	}

	QVariant BusMonitorModel::data(const QModelIndex &index, int role) const
	{
		if (!index.isValid() || index.row() < 0 || index.row() >= rows.size())
		{
			return {};
		}
		const auto &row = rows.at(index.row());
		switch (role)
		{
			case TimestampRole:
				return row.timestamp;
			case DirectionRole:
				return row.direction;
			case PgnRole:
				return row.pgn;
			case SourceRole:
				return row.source;
			case DestinationRole:
				return row.destination;
			case LengthRole:
				return row.length;
			case DataRole:
				return row.data;
			default:
				return {};
		}
	}

	QHash<int, QByteArray> BusMonitorModel::roleNames() const
	{
		return {
			{ TimestampRole, "frameTime" },
			{ DirectionRole, "frameDir" },
			{ PgnRole, "framePgn" },
			{ SourceRole, "frameSrc" },
			{ DestinationRole, "frameDst" },
			{ LengthRole, "frameLen" },
			{ DataRole, "frameData" },
		};
	}

	void BusMonitorModel::addRow(const BusFrameRow &row)
	{
		if (rows.size() >= MAX_ROWS)
		{
			beginRemoveRows(QModelIndex(), 0, 0);
			rows.removeFirst();
			endRemoveRows();
		}
		beginInsertRows(QModelIndex(), rows.size(), rows.size());
		rows.push_back(row);
		endInsertRows();
	}

	void BusMonitorModel::clear()
	{
		beginResetModel();
		rows.clear();
		endResetModel();
	}

	LogModel::LogModel(QObject *parent) :
	  QAbstractListModel(parent)
	{
	}

	int LogModel::rowCount(const QModelIndex &parent) const
	{
		return parent.isValid() ? 0 : lines.size();
	}

	QVariant LogModel::data(const QModelIndex &index, int role) const
	{
		if (!index.isValid() || (LineRole != role) || index.row() < 0 || index.row() >= lines.size())
		{
			return {};
		}
		return lines.at(index.row());
	}

	QHash<int, QByteArray> LogModel::roleNames() const
	{
		return { { LineRole, "logLine" } };
	}

	void LogModel::addLine(const QString &line)
	{
		if (lines.size() >= MAX_LINES)
		{
			beginRemoveRows(QModelIndex(), 0, 0);
			lines.removeFirst();
			endRemoveRows();
		}
		beginInsertRows(QModelIndex(), lines.size(), lines.size());
		lines.push_back(line);
		endInsertRows();
	}

	void LogModel::clear()
	{
		beginResetModel();
		lines.clear();
		endResetModel();
	}
} // namespace agisotc
