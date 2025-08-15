#include "Protocol.h"
#include <chrono>

namespace Protocol {

nlohmann::json create_message(const std::string& type, const nlohmann::json& data) {
    nlohmann::json message;
    message["type"] = type;
    message["data"] = data;
    message["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return message;
}

std::pair<std::string, nlohmann::json> parse_message(const std::string& message) {
    nlohmann::json parsed = nlohmann::json::parse(message);
    
    std::string type = parsed["type"];
    nlohmann::json data = parsed.contains("data") ? parsed["data"] : nlohmann::json{};
    
    return {type, data};
}

} // namespace Protocol