#ifndef USER_H
#define USER_H

#include <string>
#include <chrono>
#include <nlohmann/json.hpp>
// Add this include at the top of shared/User.h
#include <openssl/sha.h>

class User {
public:
    int id;
    std::string username;
    std::string email;
    std::string password_hash;
    std::string display_name;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_login;
    bool is_active;

    User();
    User(const std::string& username, const std::string& email, 
         const std::string& password_hash, const std::string& display_name = "");

    // JSON serialization (without password for security)
    nlohmann::json to_json() const;
    static User from_json(const nlohmann::json& j);
    
    // Password utilities
    static std::string hash_password(const std::string& password);
    bool verify_password(const std::string& password) const;
    
    // Validation
    static bool is_valid_username(const std::string& username);
    static bool is_valid_email(const std::string& email);
    static bool is_valid_password(const std::string& password);
};

// Authentication token structure
struct AuthToken {
    std::string token;
    int user_id;
    std::chrono::system_clock::time_point expires_at;
    
    bool is_valid() const;
    nlohmann::json to_json() const;
    static AuthToken from_json(const nlohmann::json& j);
};

#endif // USER_H