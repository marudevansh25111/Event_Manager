#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <nlohmann/json.hpp>

namespace Protocol {
    // Message types
    const std::string EVENT_CREATE = "event_create";
    const std::string EVENT_UPDATE = "event_update";
    const std::string EVENT_DELETE = "event_delete";
    const std::string EVENT_LIST = "event_list";
    const std::string REMINDER = "reminder";
    const std::string CLIENT_CONNECT = "client_connect";
    const std::string CLIENT_DISCONNECT = "client_disconnect";
    const std::string HEARTBEAT = "heartbeat";

    // Create protocol message
    nlohmann::json create_message(const std::string& type, const nlohmann::json& data = {});
    
    // Parse protocol message
    std::pair<std::string, nlohmann::json> parse_message(const std::string& message);
}

#endif // PROTOCOL_H