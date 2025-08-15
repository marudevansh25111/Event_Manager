#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include "Event.h"

class Database {
public:
    Database(const std::string& db_path);
    ~Database();
    
    bool initialize();
    
    // Event operations
    int create_event(const Event& event);
    bool update_event(const Event& event);
    bool delete_event(int event_id);
    std::vector<Event> get_all_events();
    std::vector<Event> get_events_needing_reminder();
    Event get_event_by_id(int id);
    
private:
    sqlite3* m_db;
    std::string m_db_path;
    
    bool execute_sql(const std::string& sql);
    Event event_from_row(sqlite3_stmt* stmt);
};

#endif // DATABASE_H