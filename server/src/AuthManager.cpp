#include "AuthManager.h"
#include <random>
#include <iomanip>
#include <sstream>
#include <iostream>

AuthManager::AuthManager(Database* database) : m_database(database) {
}

AuthManager::~AuthManager() {
    cleanup_expired_tokens();
}

AuthToken AuthManager::login(const std::string& username, const std::string& password) {
    // Get user from database
    User user = m_database->get_user_by_username(username);
    if (user.id == 0) {
        return AuthToken{}; // User not found
    }
    
    // Verify password
    if (!user.verify_password(password)) {
        return AuthToken{}; // Invalid password
    }
    
    // Check if user is active
    if (!user.is_active) {
        return AuthToken{}; // User account disabled
    }
    
    // Update last login
    user.last_login = std::chrono::system_clock::now();
    m_database->update_user_last_login(user.id);
    
    // Generate and store auth token
    AuthToken token = create_auth_token(user.id);
    {
        std::lock_guard<std::mutex> lock(m_tokens_mutex);
        m_active_tokens[token.token] = token;
    }
    
    std::cout << "User logged in: " << username << " (ID: " << user.id << ")" << std::endl;
    return token;
}

bool AuthManager::register_user(const std::string& username, const std::string& email, 
                               const std::string& password, const std::string& display_name) {
    // Validate input
    if (!User::is_valid_username(username)) {
        std::cerr << "Invalid username: " << username << std::endl;
        return false;
    }
    
    if (!User::is_valid_email(email)) {
        std::cerr << "Invalid email: " << email << std::endl;
        return false;
    }
    
    if (!User::is_valid_password(password)) {
        std::cerr << "Invalid password (too weak)" << std::endl;
        return false;
    }
    
    // Check if username or email already exists
    if (user_exists(username)) {
        std::cerr << "Username already exists: " << username << std::endl;
        return false;
    }
    
    if (email_exists(email)) {
        std::cerr << "Email already exists: " << email << std::endl;
        return false;
    }
    
    // Hash password and create user
    std::string password_hash = User::hash_password(password);
    User user(username, email, password_hash, display_name);
    
    // Save to database
    int user_id = m_database->create_user(user);
    if (user_id > 0) {
        std::cout << "User registered: " << username << " (ID: " << user_id << ")" << std::endl;
        return true;
    }
    
    return false;
}

bool AuthManager::logout(const std::string& token) {
    std::lock_guard<std::mutex> lock(m_tokens_mutex);
    auto it = m_active_tokens.find(token);
    if (it != m_active_tokens.end()) {
        std::cout << "User logged out (ID: " << it->second.user_id << ")" << std::endl;
        m_active_tokens.erase(it);
        return true;
    }
    return false;
}

bool AuthManager::validate_token(const std::string& token) {
    std::lock_guard<std::mutex> lock(m_tokens_mutex);
    auto it = m_active_tokens.find(token);
    if (it != m_active_tokens.end()) {
        return it->second.is_valid();
    }
    return false;
}

AuthToken AuthManager::refresh_token(const std::string& old_token) {
    std::lock_guard<std::mutex> lock(m_tokens_mutex);
    auto it = m_active_tokens.find(old_token);
    if (it != m_active_tokens.end() && it->second.is_valid()) {
        // Create new token for same user
        AuthToken new_token = create_auth_token(it->second.user_id);
        m_active_tokens.erase(it); // Remove old token
        m_active_tokens[new_token.token] = new_token;
        return new_token;
    }
    return AuthToken{};
}

User AuthManager::get_user_by_token(const std::string& token) {
    int user_id = get_user_id_by_token(token);
    if (user_id > 0) {
        return m_database->get_user_by_id(user_id);
    }
    return User{};
}

int AuthManager::get_user_id_by_token(const std::string& token) {
    std::lock_guard<std::mutex> lock(m_tokens_mutex);
    auto it = m_active_tokens.find(token);
    if (it != m_active_tokens.end() && it->second.is_valid()) {
        return it->second.user_id;
    }
    return 0;
}

bool AuthManager::user_exists(const std::string& username) {
    User user = m_database->get_user_by_username(username);
    return user.id > 0;
}

bool AuthManager::email_exists(const std::string& email) {
    User user = m_database->get_user_by_email(email);
    return user.id > 0;
}

User AuthManager::get_user_by_id(int user_id) {
    return m_database->get_user_by_id(user_id);
}

User AuthManager::get_user_by_username(const std::string& username) {
    return m_database->get_user_by_username(username);
}

void AuthManager::cleanup_expired_tokens() {
    std::lock_guard<std::mutex> lock(m_tokens_mutex);
    auto it = m_active_tokens.begin();
    while (it != m_active_tokens.end()) {
        if (!it->second.is_valid()) {
            it = m_active_tokens.erase(it);
        } else {
            ++it;
        }
    }
}

std::string AuthManager::generate_token() {
    // Generate random token
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    
    return ss.str();
}

AuthToken AuthManager::create_auth_token(int user_id) {
    AuthToken token;
    token.token = generate_token();
    token.user_id = user_id;
    token.expires_at = std::chrono::system_clock::now() + std::chrono::hours(24); // 24 hour expiry
    return token;
}

void AuthManager::remove_token(const std::string& token) {
    std::lock_guard<std::mutex> lock(m_tokens_mutex);
    m_active_tokens.erase(token);
}