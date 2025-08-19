#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include "Event.h"
#include "User.h"

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
    std::vector<Event> get_events_for_user(int user_id);
    Event get_event_by_id(int id);
    
    // User operations
    int create_user(const User& user);
    bool update_user(const User& user);
    bool update_user_last_login(int user_id);
    User get_user_by_id(int id);
    User get_user_by_username(const std::string& username);
    User get_user_by_email(const std::string& email);
    bool delete_user(int user_id);
    
private:
    sqlite3* m_db;
    std::string m_db_path;
    
    bool execute_sql(const std::string& sql);
    Event event_from_row(sqlite3_stmt* stmt);
    User user_from_row(sqlite3_stmt* stmt);
};

#endif // DATABASE_H