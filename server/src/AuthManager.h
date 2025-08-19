// AuthManager.h
#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include "User.h"
#include "Database.h"

class AuthManager {
public:
    explicit AuthManager(Database* database);
    ~AuthManager();
    
    // Authentication operations
    AuthToken login(const std::string& username, const std::string& password);
    bool register_user(const std::string& username, const std::string& email, 
                      const std::string& password, const std::string& display_name = "");
    bool logout(const std::string& token);
    
    // Token management
    bool validate_token(const std::string& token);
    AuthToken refresh_token(const std::string& old_token);
    User get_user_by_token(const std::string& token);
    int get_user_id_by_token(const std::string& token);
    
    // User management
    bool user_exists(const std::string& username);
    bool email_exists(const std::string& email);
    User get_user_by_id(int user_id);
    User get_user_by_username(const std::string& username);
    
    // Session cleanup
    void cleanup_expired_tokens();
    
private:
    Database* m_database;
    std::unordered_map<std::string, AuthToken> m_active_tokens;
    std::mutex m_tokens_mutex;
    
    std::string generate_token();
    AuthToken create_auth_token(int user_id);
    void remove_token(const std::string& token);
};

#endif // AUTH_MANAGER_H