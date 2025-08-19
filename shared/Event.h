#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <chrono>
#include <nlohmann/json.hpp>

class Event {
public:
    int id;
    int user_id;  // NEW: Associate event with user
    std::string title;
    std::string description;
    std::chrono::system_clock::time_point event_time;
    std::chrono::system_clock::time_point reminder_time;
    std::string creator;
    bool reminder_sent;
    std::chrono::system_clock::time_point created_at;

    Event();
    Event(int user_id, const std::string& title, const std::string& description, 
          const std::chrono::system_clock::time_point& event_time,
          const std::string& creator = "");

    // JSON serialization
    nlohmann::json to_json() const;
    static Event from_json(const nlohmann::json& j);
    
    // Utility functions
    std::string get_formatted_time() const;
    bool needs_reminder() const;
    std::chrono::minutes time_until_event() const;
};

#endif // EVENT_H