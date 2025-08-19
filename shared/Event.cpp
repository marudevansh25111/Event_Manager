#include "Event.h"
#include <iomanip>
#include <sstream>

Event::Event() : id(0), user_id(0), reminder_sent(false) {
    auto now = std::chrono::system_clock::now();
    created_at = now;
    event_time = now + std::chrono::hours(1); // Default 1 hour from now
    reminder_time = event_time - std::chrono::hours(1); // 1 hour before event
}

Event::Event(int user_id, const std::string& title, const std::string& description, 
             const std::chrono::system_clock::time_point& event_time,
             const std::string& creator)
    : id(0), user_id(user_id), title(title), description(description), event_time(event_time),
      creator(creator), reminder_sent(false) {
    
    created_at = std::chrono::system_clock::now();
    reminder_time = event_time - std::chrono::hours(1); // Default 1 hour before
}

nlohmann::json Event::to_json() const {
    auto event_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        event_time.time_since_epoch()).count();
    auto reminder_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        reminder_time.time_since_epoch()).count();
    auto created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        created_at.time_since_epoch()).count();

    return nlohmann::json{
        {"id", id},
        {"user_id", user_id},
        {"title", title},
        {"description", description},
        {"event_time", event_time_ms},
        {"reminder_time", reminder_time_ms},
        {"creator", creator},
        {"reminder_sent", reminder_sent},
        {"created_at", created_at_ms}
    };
}

Event Event::from_json(const nlohmann::json& j) {
    Event event;
    event.id = j["id"];
    event.user_id = j.contains("user_id") ? j["user_id"].get<int>() : 0;
    event.title = j["title"];
    event.description = j["description"];
    event.creator = j["creator"];
    event.reminder_sent = j["reminder_sent"];
    
    auto event_time_ms = j["event_time"].get<int64_t>();
    auto reminder_time_ms = j["reminder_time"].get<int64_t>();
    auto created_at_ms = j["created_at"].get<int64_t>();
    
    event.event_time = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(event_time_ms));
    event.reminder_time = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(reminder_time_ms));
    event.created_at = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(created_at_ms));
    
    return event;
}

std::string Event::get_formatted_time() const {
    auto time_t = std::chrono::system_clock::to_time_t(event_time);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool Event::needs_reminder() const {
    if (reminder_sent) return false;
    auto now = std::chrono::system_clock::now();
    return now >= reminder_time && now < event_time;
}

std::chrono::minutes Event::time_until_event() const {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::minutes>(event_time - now);
}