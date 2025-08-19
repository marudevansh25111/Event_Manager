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
    
    // Create users table
    const std::string create_users_table = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            email TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            display_name TEXT NOT NULL,
            created_at INTEGER NOT NULL,
            last_login INTEGER NOT NULL,
            is_active INTEGER DEFAULT 1
        );
    )";
    
    // Create events table with user_id
    const std::string create_events_table = R"(
        CREATE TABLE IF NOT EXISTS events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            title TEXT NOT NULL,
            description TEXT,
            event_time INTEGER NOT NULL,
            reminder_time INTEGER NOT NULL,
            creator TEXT,
            reminder_sent INTEGER DEFAULT 0,
            created_at INTEGER NOT NULL,
            FOREIGN KEY (user_id) REFERENCES users (id)
        );
    )";
    
    return execute_sql(create_users_table) && execute_sql(create_events_table);
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
        INSERT INTO events (user_id, title, description, event_time, reminder_time, creator, reminder_sent, created_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?);
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
    
    sqlite3_bind_int(stmt, 1, event.user_id);
    sqlite3_bind_text(stmt, 2, event.title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, event.description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, event_time_ms);
    sqlite3_bind_int64(stmt, 5, reminder_time_ms);
    sqlite3_bind_text(stmt, 6, event.creator.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, event.reminder_sent ? 1 : 0);
    sqlite3_bind_int64(stmt, 8, created_at_ms);
    
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

//adding get_event_by_id method to this file

// Add this function to your Database.cpp file

Event Database::get_event_by_id(int id) {
    const std::string sql = "SELECT * FROM events WHERE id = ?;";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        return Event{};
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    Event event;
    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        event = event_from_row(stmt);
    }
    
    sqlite3_finalize(stmt);
    return event;
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

// Event Database::event_from_row(sqlite3_stmt* stmt) {
//     Event event;
    
//     event.id = sqlite3_column_int(stmt, 0);
//     event.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
//     event.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    
//     auto event_time_ms = sqlite3_column_int64(stmt, 3);
//     auto reminder_time_ms = sqlite3_column_int64(stmt, 4);
    
//     if (sqlite3_column_text(stmt, 5)) {
//         event.creator = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
//     }
    
//     event.reminder_sent = sqlite3_column_int(stmt, 6) == 1;
//     auto created_at_ms = sqlite3_column_int64(stmt, 7);
    
//     event.event_time = std::chrono::system_clock::time_point(
//         std::chrono::milliseconds(event_time_ms));
//     event.reminder_time = std::chrono::system_clock::time_point(
//         std::chrono::milliseconds(reminder_time_ms));
//     event.created_at = std::chrono::system_clock::time_point(
//         std::chrono::milliseconds(created_at_ms));
    
//     return event;
// }

//chages to add my files
// Add these methods to the end of Database.cpp file

std::vector<Event> Database::get_events_for_user(int user_id) {
    std::vector<Event> events;
    const std::string sql = "SELECT * FROM events WHERE user_id = ? ORDER BY event_time ASC;";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        return events;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        events.push_back(event_from_row(stmt));
    }
    
    sqlite3_finalize(stmt);
    return events;
}

int Database::create_user(const User& user) {
    const std::string sql = R"(
        INSERT INTO users (username, email, password_hash, display_name, created_at, last_login, is_active)
        VALUES (?, ?, ?, ?, ?, ?, ?);
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare user insert statement: " << sqlite3_errmsg(m_db) << std::endl;
        return -1;
    }
    
    auto created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        user.created_at.time_since_epoch()).count();
    auto last_login_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        user.last_login.time_since_epoch()).count();
    
    sqlite3_bind_text(stmt, 1, user.username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user.email.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user.password_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, user.display_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, created_at_ms);
    sqlite3_bind_int64(stmt, 6, last_login_ms);
    sqlite3_bind_int(stmt, 7, user.is_active ? 1 : 0);
    
    rc = sqlite3_step(stmt);
    int user_id = -1;
    
    if (rc == SQLITE_DONE) {
        user_id = static_cast<int>(sqlite3_last_insert_rowid(m_db));
    } else {
        std::cerr << "Failed to insert user: " << sqlite3_errmsg(m_db) << std::endl;
    }
    
    sqlite3_finalize(stmt);
    return user_id;
}

bool Database::update_user(const User& user) {
    const std::string sql = R"(
        UPDATE users 
        SET username = ?, email = ?, password_hash = ?, display_name = ?, 
            last_login = ?, is_active = ?
        WHERE id = ?;
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare user update statement: " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }
    
    auto last_login_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        user.last_login.time_since_epoch()).count();
    
    sqlite3_bind_text(stmt, 1, user.username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user.email.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user.password_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, user.display_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, last_login_ms);
    sqlite3_bind_int(stmt, 6, user.is_active ? 1 : 0);
    sqlite3_bind_int(stmt, 7, user.id);
    
    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE);
    
    sqlite3_finalize(stmt);
    return success;
}

bool Database::update_user_last_login(int user_id) {
    const std::string sql = "UPDATE users SET last_login = ? WHERE id = ?;";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        return false;
    }
    
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    sqlite3_bind_int64(stmt, 1, now_ms);
    sqlite3_bind_int(stmt, 2, user_id);
    
    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE);
    
    sqlite3_finalize(stmt);
    return success;
}

User Database::get_user_by_id(int id) {
    const std::string sql = "SELECT * FROM users WHERE id = ?;";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        return User{};
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    User user;
    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        user = user_from_row(stmt);
    }
    
    sqlite3_finalize(stmt);
    return user;
}

User Database::get_user_by_username(const std::string& username) {
    const std::string sql = "SELECT * FROM users WHERE username = ?;";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        return User{};
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    
    User user;
    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        user = user_from_row(stmt);
    }
    
    sqlite3_finalize(stmt);
    return user;
}

User Database::get_user_by_email(const std::string& email) {
    const std::string sql = "SELECT * FROM users WHERE email = ?;";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        return User{};
    }
    
    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_STATIC);
    
    User user;
    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        user = user_from_row(stmt);
    }
    
    sqlite3_finalize(stmt);
    return user;
}

bool Database::delete_user(int user_id) {
    const std::string sql = "DELETE FROM users WHERE id = ?;";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE);
    
    sqlite3_finalize(stmt);
    return success;
}

User Database::user_from_row(sqlite3_stmt* stmt) {
    User user;
    
    user.id = sqlite3_column_int(stmt, 0);
    user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    user.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    user.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    user.display_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    
    auto created_at_ms = sqlite3_column_int64(stmt, 5);
    auto last_login_ms = sqlite3_column_int64(stmt, 6);
    
    user.is_active = sqlite3_column_int(stmt, 7) == 1;
    
    user.created_at = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(created_at_ms));
    user.last_login = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(last_login_ms));
    
    return user;
}

// Also update the event_from_row method to handle user_id
Event Database::event_from_row(sqlite3_stmt* stmt) {
    Event event;
    
    event.id = sqlite3_column_int(stmt, 0);
    event.user_id = sqlite3_column_int(stmt, 1);  // NEW: user_id column
    event.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    event.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    
    auto event_time_ms = sqlite3_column_int64(stmt, 4);
    auto reminder_time_ms = sqlite3_column_int64(stmt, 5);
    
    if (sqlite3_column_text(stmt, 6)) {
        event.creator = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    }
    
    event.reminder_sent = sqlite3_column_int(stmt, 7) == 1;
    auto created_at_ms = sqlite3_column_int64(stmt, 8);
    
    event.event_time = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(event_time_ms));
    event.reminder_time = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(reminder_time_ms));
    event.created_at = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(created_at_ms));
    
    return event;
}