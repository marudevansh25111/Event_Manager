#ifndef EVENTMODEL_H
#define EVENTMODEL_H

#include <QAbstractTableModel>
#include <vector>
#include "Event.h"

class EventModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns {
        TitleColumn = 0,
        DescriptionColumn,
        EventTimeColumn,
        CreatorColumn,
        ColumnCount
    };

    explicit EventModel(QObject *parent = nullptr);

    // QAbstractTableModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Event management
    void setEvents(const std::vector<Event>& events);
    void addEvent(const Event& event);
    void updateEvent(const Event& event);
    void removeEvent(int eventId);
    Event getEvent(int row) const;
    void clear();

private:
    std::vector<Event> m_events;
    int findEventIndex(int eventId) const;
};

#endif // EVENTMODEL_H