#include "EventModel.h"
#include <QDateTime>
#include <algorithm>

EventModel::EventModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int EventModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return static_cast<int>(m_events.size());
}

int EventModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return ColumnCount;
}

QVariant EventModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(m_events.size()))
        return QVariant();

    const Event& event = m_events[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case TitleColumn:
            return QString::fromStdString(event.title);
        case DescriptionColumn:
            return QString::fromStdString(event.description);
        case EventTimeColumn:
            return QString::fromStdString(event.get_formatted_time());
        case CreatorColumn:
            return QString::fromStdString(event.creator);
        }
    }
    else if (role == Qt::ToolTipRole) {
        return QString("Event: %1\nDescription: %2\nTime: %3\nCreator: %4")
                .arg(QString::fromStdString(event.title))
                .arg(QString::fromStdString(event.description))
                .arg(QString::fromStdString(event.get_formatted_time()))
                .arg(QString::fromStdString(event.creator));
    }

    return QVariant();
}

QVariant EventModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case TitleColumn:
        return "Title";
    case DescriptionColumn:
        return "Description";
    case EventTimeColumn:
        return "Event Time";
    case CreatorColumn:
        return "Creator";
    }

    return QVariant();
}

void EventModel::setEvents(const std::vector<Event>& events)
{
    beginResetModel();
    m_events = events;
    // Sort by event time
    std::sort(m_events.begin(), m_events.end(), 
              [](const Event& a, const Event& b) {
                  return a.event_time < b.event_time;
              });
    endResetModel();
}

void EventModel::addEvent(const Event& event)
{
    // Find insertion point to maintain sorted order
    auto it = std::upper_bound(m_events.begin(), m_events.end(), event,
                              [](const Event& a, const Event& b) {
                                  return a.event_time < b.event_time;
                              });
    
    int row = static_cast<int>(std::distance(m_events.begin(), it));
    
    beginInsertRows(QModelIndex(), row, row);
    m_events.insert(it, event);
    endInsertRows();
}

void EventModel::updateEvent(const Event& event)
{
    int index = findEventIndex(event.id);
    if (index >= 0) {
        m_events[index] = event;
        
        // Re-sort if necessary
        bool needsResort = false;
        if (index > 0 && m_events[index-1].event_time > event.event_time) {
            needsResort = true;
        }
        if (index < static_cast<int>(m_events.size()-1) && 
            m_events[index+1].event_time < event.event_time) {
            needsResort = true;
        }
        
        if (needsResort) {
            beginResetModel();
            std::sort(m_events.begin(), m_events.end(), 
                      [](const Event& a, const Event& b) {
                          return a.event_time < b.event_time;
                      });
            endResetModel();
        } else {
            // Just update the row
            QModelIndex topLeft = createIndex(index, 0);
            QModelIndex bottomRight = createIndex(index, ColumnCount - 1);
            emit dataChanged(topLeft, bottomRight);
        }
    } else {
        // Event doesn't exist, add it
        addEvent(event);
    }
}

void EventModel::removeEvent(int eventId)
{
    int index = findEventIndex(eventId);
    if (index >= 0) {
        beginRemoveRows(QModelIndex(), index, index);
        m_events.erase(m_events.begin() + index);
        endRemoveRows();
    }
}

Event EventModel::getEvent(int row) const
{
    if (row >= 0 && row < static_cast<int>(m_events.size())) {
        return m_events[row];
    }
    return Event();
}

void EventModel::clear()
{
    beginResetModel();
    m_events.clear();
    endResetModel();
}

int EventModel::findEventIndex(int eventId) const
{
    for (size_t i = 0; i < m_events.size(); ++i) {
        if (m_events[i].id == eventId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}