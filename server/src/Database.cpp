#include "Database.h"
#include <iostream>
#include <chrono>

Database::Database(const std::string& db_path) : m_db(nullptr), m_db_path(db_path) {
    initialize();
}

Database::~Database() {
    if (m_db) {
        sqlite3_close(m_db);
    }
}

bool Database::initialize() {
    int rc = sqlite3_open(m_db_path.c_str(), &m_db);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }
    
    // Create events table
    const std::string create_table_sql = R"(
        CREATE TABLE IF NOT EXISTS events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            description TEXT,
            event_time INTEGER NOT NULL,
            reminder_time INTEGER NOT NULL,
            creator TEXT,
            reminder_sent INTEGER DEFAULT 0,
            created_at INTEGER NOT NULL
        );
    )";
    
    return execute_sql(create_table_sql);
}

bool Database::execute_sql(const std::string& sql) {
    char* error_msg = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &error_msg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << error_msg << std::endl;
        sqlite3_free(error_msg);
        return false;
    }
    
    return true;
}

int Database::create_event(const Event& event) {
    const std::string sql = R"(
        INSERT INTO events (title, description, event_time, reminder_time, creator, reminder_sent, created_at)
        VALUES (?, ?, ?, ?, ?, ?, ?);
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(m_db) << std::endl;
        return -1;
    }
    
    auto event_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        event.event_time.time_since_epoch()).count();
    auto reminder_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        event.reminder_time.time_since_epoch()).count();
    auto created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        event.created_at.time_since_epoch()).count();
    
    sqlite3_bind_text(stmt, 1, event.title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, event.description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, event_time_ms);
    sqlite3_bind_int64(stmt, 4, reminder_time_ms);
    sqlite3_bind_text(stmt, 5, event.creator.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, event.reminder_sent ? 1 : 0);
    sqlite3_bind_int64(stmt, 7, created_at_ms);
    
    rc = sqlite3_step(stmt);
    int event_id = -1;
    
    if (rc == SQLITE_DONE) {
        event_id = static_cast<int>(sqlite3_last_insert_rowid(m_db));
    } else {
        std::cerr << "Failed to insert event: " << sqlite3_errmsg(m_db) << std::endl;
    }
    
    sqlite3_finalize(stmt);
    return event_id;
}

bool Database::update_event(const Event& event) {
    const std::string sql = R"(
        UPDATE events 
        SET title = ?, description = ?, event_time = ?, reminder_time = ?, 
            creator = ?, reminder_sent = ?
        WHERE id = ?;
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare update statement: " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }
    
    auto event_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        event.event_time.time_since_epoch()).count();
    auto reminder_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        event.reminder_time.time_since_epoch()).count();
    
    sqlite3_bind_text(stmt, 1, event.title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, event.description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, event_time_ms);
    sqlite3_bind_int64(stmt, 4, reminder_time_ms);
    sqlite3_bind_text(stmt, 5, event.creator.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, event.reminder_sent ? 1 : 0);
    sqlite3_bind_int(stmt, 7, event.id);
    
    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE);
    
    sqlite3_finalize(stmt);
    return success;
}

bool Database::delete_event(int event_id) {
    const std::string sql = "DELETE FROM events WHERE id = ?;";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, event_id);
    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE);
    
    sqlite3_finalize(stmt);
    return success;
}

std::vector<Event> Database::get_all_events() {
    std::vector<Event> events;
    const std::string sql = "SELECT * FROM events ORDER BY event_time ASC;";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        return events;
    }
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        events.push_back(event_from_row(stmt));
    }
    
    sqlite3_finalize(stmt);
    return events;
}

std::vector<Event> Database::get_events_needing_reminder() {
    std::vector<Event> events;
    const std::string sql = "SELECT * FROM events WHERE reminder_sent = 0 ORDER BY reminder_time ASC;";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        return events;
    }
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        events.push_back(event_from_row(stmt));
    }
    
    sqlite3_finalize(stmt);
    return events;
}

Event Database::event_from_row(sqlite3_stmt* stmt) {
    Event event;
    
    event.id = sqlite3_column_int(stmt, 0);
    event.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    event.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    
    auto event_time_ms = sqlite3_column_int64(stmt, 3);
    auto reminder_time_ms = sqlite3_column_int64(stmt, 4);
    
    if (sqlite3_column_text(stmt, 5)) {
        event.creator = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    }
    
    event.reminder_sent = sqlite3_column_int(stmt, 6) == 1;
    auto created_at_ms = sqlite3_column_int64(stmt, 7);
    
    event.event_time = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(event_time_ms));
    event.reminder_time = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(reminder_time_ms));
    event.created_at = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(created_at_ms));
    
    return event;
}