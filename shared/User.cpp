#include "User.h"
#include <iomanip>
#include <sstream>
#include <regex>
#include <random>
#include <openssl/sha.h>

User::User() : id(0), is_active(true) {
    created_at = std::chrono::system_clock::now();
    last_login = created_at;
}

User::User(const std::string& username, const std::string& email, 
           const std::string& password_hash, const std::string& display_name)
    : id(0), username(username), email(email), password_hash(password_hash),
      display_name(display_name.empty() ? username : display_name), is_active(true) {
    
    created_at = std::chrono::system_clock::now();
    last_login = created_at;
}

nlohmann::json User::to_json() const {
    auto created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        created_at.time_since_epoch()).count();
    auto last_login_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        last_login.time_since_epoch()).count();

    return nlohmann::json{
        {"id", id},
        {"username", username},
        {"email", email},
        {"display_name", display_name},
        {"created_at", created_at_ms},
        {"last_login", last_login_ms},
        {"is_active", is_active}
        // Note: password_hash is NOT included for security
    };
}

User User::from_json(const nlohmann::json& j) {
    User user;
    user.id = j["id"];
    user.username = j["username"];
    user.email = j["email"];
    user.display_name = j["display_name"];
    user.is_active = j["is_active"];
    
    auto created_at_ms = j["created_at"].get<int64_t>();
    auto last_login_ms = j["last_login"].get<int64_t>();
    
    user.created_at = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(created_at_ms));
    user.last_login = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(last_login_ms));
    
    return user;
}

std::string User::hash_password(const std::string& password) {
    // Generate salt (16 random bytes)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    unsigned char salt[16];
    for (int i = 0; i < 16; ++i) {
        salt[i] = static_cast<unsigned char>(dis(gen));
    }
    
    // Hash password with salt using SHA-256
    std::string salted_password = std::string(reinterpret_cast<const char*>(salt), 16) + password;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(salted_password.c_str()), 
           salted_password.length(), hash);
    
    // Convert salt and hash to hex string
    std::stringstream ss;
    
    // Add salt as hex
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(salt[i]);
    }
    ss << ":";
    
    // Add hash as hex
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

bool User::verify_password(const std::string& password) const {
    // Extract salt from stored hash
    size_t colon_pos = password_hash.find(':');
    if (colon_pos == std::string::npos) return false;
    
    std::string salt_hex = password_hash.substr(0, colon_pos);
    std::string stored_hash = password_hash.substr(colon_pos + 1);
    
    // Validate hex lengths
    if (salt_hex.length() != 32 || stored_hash.length() != 64) {
        return false; // Invalid format
    }
    
    // Convert salt from hex to bytes
    unsigned char salt[16];
    for (size_t i = 0; i < 16; ++i) {
        std::string byte_str = salt_hex.substr(i * 2, 2);
        salt[i] = static_cast<unsigned char>(std::strtol(byte_str.c_str(), nullptr, 16));
    }
    
    // Hash provided password with extracted salt
    std::string salted_password = std::string(reinterpret_cast<const char*>(salt), 16) + password;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(salted_password.c_str()), 
           salted_password.length(), hash);
    
    // Convert computed hash to hex string
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str() == stored_hash;
}

bool User::is_valid_username(const std::string& username) {
    // Username: 3-20 characters, alphanumeric and underscore only
    std::regex username_regex("^[a-zA-Z0-9_]{3,20}$");
    return std::regex_match(username, username_regex);
}

bool User::is_valid_email(const std::string& email) {
    // Basic email validation
    std::regex email_regex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    return std::regex_match(email, email_regex);
}

bool User::is_valid_password(const std::string& password) {
    // Password: at least 6 characters
    return password.length() >= 6;
}

// AuthToken implementation
bool AuthToken::is_valid() const {
    return std::chrono::system_clock::now() < expires_at;
}

nlohmann::json AuthToken::to_json() const {
    auto expires_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        expires_at.time_since_epoch()).count();
    
    return nlohmann::json{
        {"token", token},
        {"user_id", user_id},
        {"expires_at", expires_at_ms}
    };
}

AuthToken AuthToken::from_json(const nlohmann::json& j) {
    AuthToken auth_token;
    auth_token.token = j["token"];
    auth_token.user_id = j["user_id"];
    
    auto expires_at_ms = j["expires_at"].get<int64_t>();
    auth_token.expires_at = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(expires_at_ms));
    
    return auth_token;
}