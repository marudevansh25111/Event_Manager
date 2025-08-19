#include "EventServer.h"
// #include "Protocol.h"  // Make sure this is included
#include "../../shared/Protocol.h"
#include <iostream>
#include <chrono>

// WebSocketSession implementation
WebSocketSession::WebSocketSession(tcp::socket&& socket)
    : m_ws(std::move(socket)) {
}

WebSocketSession::~WebSocketSession() = default;

void WebSocketSession::run() {
    // Set suggested timeout settings for the websocket
    m_ws.set_option(websocket::stream_base::timeout::suggested(
        beast::role_type::server));

    // Set a decorator to change the Server of the handshake
    m_ws.set_option(websocket::stream_base::decorator(
        [](websocket::response_type& res) {
            res.set(http::field::server, "Event-Manager-Server");
        }));

    // Accept the websocket handshake
    m_ws.async_accept(
        beast::bind_front_handler(
            &WebSocketSession::on_accept,
            shared_from_this()));
}

void WebSocketSession::send(const std::string& message) {
    auto const ss = std::make_shared<std::string const>(std::move(message));
    
    // Post our work to the strand, this ensures that the members of `this` will not be accessed concurrently
    net::post(
        m_ws.get_executor(),
        beast::bind_front_handler(
            [self = shared_from_this(), ss]() {
                // Always add to queue
                self->m_queue.push_back(ss);

                // Are we already writing?
                if(self->m_queue.size() > 1)
                    return;

                // We are not currently writing, so send this immediately
                self->m_ws.async_write(
                    net::buffer(*self->m_queue.front()),
                    beast::bind_front_handler(
                        &WebSocketSession::on_write,
                        self));
            }));
}

void WebSocketSession::close() {
    m_ws.async_close(websocket::close_code::normal,
        [self = shared_from_this()](beast::error_code ec) {
            if(ec) {
                std::cerr << "WebSocket close error: " << ec.message() << std::endl;
            }
        });
}

void WebSocketSession::set_message_handler(
    std::function<void(std::shared_ptr<WebSocketSession>, const std::string&)> handler) {
    m_message_handler = handler;
}

void WebSocketSession::set_close_handler(
    std::function<void(std::shared_ptr<WebSocketSession>)> handler) {
    m_close_handler = handler;
}

void WebSocketSession::set_connect_handler(
    std::function<void(std::shared_ptr<WebSocketSession>)> handler) {
    m_connect_handler = handler;
}

void WebSocketSession::on_accept(beast::error_code ec) {
    if(ec) {
        std::cerr << "WebSocket accept error: " << ec.message() << std::endl;
        return;
    }

    std::cout << "WebSocket handshake completed successfully!" << std::endl;
    
    // Notify that connection is fully established
    if(m_connect_handler) {
        m_connect_handler(shared_from_this());
    }

    // Read a message
    do_read();
}

void WebSocketSession::do_read() {
    // Read a message into our buffer
    m_ws.async_read(
        m_buffer,
        beast::bind_front_handler(
            &WebSocketSession::on_read,
            shared_from_this()));
}

void WebSocketSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This indicates that the session was closed
    if(ec == websocket::error::closed) {
        if(m_close_handler) {
            m_close_handler(shared_from_this());
        }
        return;
    }

    if(ec) {
        std::cerr << "WebSocket read error: " << ec.message() << std::endl;
        if(m_close_handler) {
            m_close_handler(shared_from_this());
        }
        return;
    }

    // Handle the message
    if(m_message_handler) {
        std::string message = beast::buffers_to_string(m_buffer.data());
        m_message_handler(shared_from_this(), message);
    }

    // Clear the buffer
    m_buffer.consume(m_buffer.size());

    // Do another read
    do_read();
}

void WebSocketSession::on_write(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if(ec) {
        std::cerr << "WebSocket write error: " << ec.message() << std::endl;
        return;
    }

    // Remove the string from the queue
    m_queue.erase(m_queue.begin());

    // Send the next message if any
    if(!m_queue.empty()) {
        m_ws.async_write(
            net::buffer(*m_queue.front()),
            beast::bind_front_handler(
                &WebSocketSession::on_write,
                shared_from_this()));
    }
}

// EventServer implementation
EventServer::EventServer() 
    : m_ioc(1)
    , m_acceptor(m_ioc)
    , m_running(false) {
    
    m_database = std::make_unique<Database>("events.db");
    m_reminderManager = std::make_unique<ReminderManager>(m_database.get());
    m_authManager = std::make_unique<AuthManager>(m_database.get());
    
    // Setup reminder callback
    m_reminderManager->setReminderCallback([this](const Event& event) {
        send_reminder(event);
    });
}

EventServer::~EventServer() {
    stop();
}

void EventServer::start(int port) {
    try {
        auto const address = net::ip::make_address("0.0.0.0");
        
        // Open the acceptor
        m_acceptor.open(tcp::v4());
        
        // Allow address reuse
        m_acceptor.set_option(net::socket_base::reuse_address(true));
        
        // Bind to the server address
        m_acceptor.bind({address, static_cast<unsigned short>(port)});
        
        // Start listening for connections
        m_acceptor.listen(net::socket_base::max_listen_connections);
        
        m_running = true;
        m_reminderManager->start();
        
        std::cout << "Event Manager Server started on port " << port << std::endl;
        
        // Start accepting connections
        do_accept();
        
        // Run the I/O service in a separate thread
        m_thread = std::thread([this]() {
            m_ioc.run();
        });
        
    } catch (const std::exception& e) {
        std::cerr << "Server start error: " << e.what() << std::endl;
    }
}

void EventServer::stop() {
    if (m_running) {
        m_running = false;
        m_reminderManager->stop();
        
        // Close all sessions
        {
            std::lock_guard<std::mutex> lock(m_sessions_lock);
            for (auto& session : m_sessions) {
                session->close();
            }
            m_sessions.clear();
        }
        
        m_ioc.stop();
        
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }
}

void EventServer::do_accept() {
    // The new connection gets its own strand
    m_acceptor.async_accept(
        net::make_strand(m_ioc),
        beast::bind_front_handler(
            &EventServer::on_accept,
            this));
}

void EventServer::on_accept(beast::error_code ec, tcp::socket socket) {
    if(ec) {
        std::cerr << "Accept error: " << ec.message() << std::endl;
        // Continue accepting even on error
        if(m_running) {
            do_accept();
        }
        return;
    }

    std::cout << "New TCP connection received, starting WebSocket handshake..." << std::endl;

    // Create the session and run it
    auto session = std::make_shared<WebSocketSession>(std::move(socket));
    
    // Set handlers BEFORE starting session
    session->set_message_handler([this](auto session, const std::string& message) {
        on_message(session, message);
    });
    
    session->set_close_handler([this](auto session) {
        on_session_close(session);
    });
    
    session->set_connect_handler([this](auto session) {
        on_connection_established(session);
    });
    
    std::cout << "WebSocket handshake starting..." << std::endl;
    
    // Start the session (this will do the WebSocket handshake)
    session->run();

    // Accept another connection
    if(m_running) {
        do_accept();
    }
}

void EventServer::on_message(std::shared_ptr<WebSocketSession> session, const std::string& message) {
    try {
        auto [type, data] = Protocol::parse_message(message);
        
        if (type == Protocol::AUTH_LOGIN) {
            handle_auth_login(session, data);
        } else if (type == Protocol::AUTH_REGISTER) {
            handle_auth_register(session, data);
        } else if (type == Protocol::AUTH_LOGOUT) {
            handle_auth_logout(session, data);
        } else if (type == Protocol::EVENT_CREATE) {
            handle_event_create(session, data);
        } else if (type == Protocol::EVENT_UPDATE) {
            handle_event_update(session, data);
        } else if (type == Protocol::EVENT_DELETE) {
            handle_event_delete(session, data);
        } else if (type == Protocol::EVENT_LIST) {
            handle_event_list(session, data);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Message handling error: " << e.what() << std::endl;
    }
}

// void EventServer::on_connection_established(std::shared_ptr<WebSocketSession> session) {
//     // Add to active sessions only after successful handshake
//     {
//         std::lock_guard<std::mutex> lock(m_sessions_lock);
//         m_sessions.insert(session);
//     }
    
//     std::cout << "Client successfully connected! Total active connections: " << m_sessions.size() << std::endl;
    
//     // Send all existing events to the new client
//     auto events = m_database->get_all_events();
//     nlohmann::json events_json = nlohmann::json::array();
    
//     for (const auto& event : events) {
//         events_json.push_back(event.to_json());
//     }
    
//     auto message = Protocol::create_message(Protocol::EVENT_LIST, events_json);
//     session->send(message.dump());
// }

void EventServer::on_connection_established(std::shared_ptr<WebSocketSession> session) {
    // Add to active sessions only after successful handshake
    {
        std::lock_guard<std::mutex> lock(m_sessions_lock);
        m_sessions.insert(session);
    }
    
    std::cout << "Client successfully connected! Total active connections: " << m_sessions.size() << std::endl;
    
    // NOTE: Don't send events here - only send after authentication
    // Events will be sent when user logs in via handle_event_list()
}



void EventServer::on_session_close(std::shared_ptr<WebSocketSession> session) {
    std::lock_guard<std::mutex> lock(m_sessions_lock);
    m_sessions.erase(session);
    std::cout << "Client disconnected. Total active connections: " << m_sessions.size() << std::endl;
}

// void EventServer::handle_event_create(std::shared_ptr<WebSocketSession> session, const nlohmann::json& data) {
//     if (!is_authenticated(session, data)) return;
    
//     try {
//         Event event = Event::from_json(data);
        
//         // Set user_id from auth token
//         std::string token = data["auth_token"];
//         int user_id = m_authManager->get_user_id_by_token(token);
//         event.user_id = user_id;
        
//         int id = m_database->create_event(event);
//         event.id = id;
        
//         broadcast_event_update(event, "created");
//         std::cout << "Event created: " << event.title << " (User: " << user_id << ")" << std::endl;
        
//     } catch (const std::exception& e) {
//         std::cerr << "Error creating event: " << e.what() << std::endl;
//     }
// }


void EventServer::handle_event_create(std::shared_ptr<WebSocketSession> session, const nlohmann::json& data) {
    if (!is_authenticated(session, data)) return;
    
    try {
        Event event = Event::from_json(data);
        
        // Set user_id from auth token (to track who created it)
        std::string token = data["auth_token"];
        int user_id = m_authManager->get_user_id_by_token(token);
        event.user_id = user_id;
        
        int id = m_database->create_event(event);
        event.id = id;
        
        // SHARED CALENDAR: Broadcast new event to ALL connected users
        broadcast_event_update(event, "created");
        std::cout << "Event created and broadcast to all users: " << event.title << " (Created by User: " << user_id << ")" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error creating event: " << e.what() << std::endl;
    }
}


// void EventServer::handle_event_update(std::shared_ptr<WebSocketSession> session, const nlohmann::json& data) {
//     if (!is_authenticated(session, data)) return;
    
//     try {
//         Event event = Event::from_json(data);
        
//         // Verify user owns this event
//         std::string token = data["auth_token"];
//         int user_id = m_authManager->get_user_id_by_token(token);
        
//         Event existing_event = m_database->get_event_by_id(event.id);
//         if (existing_event.user_id != user_id) {
//             nlohmann::json error_response = {
//                 {"error", "You can only modify your own events"},
//                 {"code", "PERMISSION_DENIED"}
//             };
//             auto message = Protocol::create_message(Protocol::AUTH_ERROR, error_response);
//             session->send(message.dump());
//             return;
//         }
        
//         event.user_id = user_id; // Ensure user_id is set
//         bool success = m_database->update_event(event);
        
//         if (success) {
//             broadcast_event_update(event, "updated");
//             std::cout << "Event updated: " << event.title << " (User: " << user_id << ")" << std::endl;
//         }
        
//     } catch (const std::exception& e) {
//         std::cerr << "Error updating event: " << e.what() << std::endl;
//     }
// }


void EventServer::handle_event_update(std::shared_ptr<WebSocketSession> session, const nlohmann::json& data) {
    if (!is_authenticated(session, data)) return;
    
    try {
        Event event = Event::from_json(data);
        
        // Get current user from token
        std::string token = data["auth_token"];
        int user_id = m_authManager->get_user_id_by_token(token);
        
        // OPTIONAL: Check if user owns this event (for edit permissions)
        Event existing_event = m_database->get_event_by_id(event.id);
        if (existing_event.user_id != user_id) {
            nlohmann::json error_response = {
                {"error", "You can only modify your own events"},
                {"code", "PERMISSION_DENIED"}
            };
            auto message = Protocol::create_message(Protocol::AUTH_ERROR, error_response);
            session->send(message.dump());
            return;
        }
        
        event.user_id = user_id; // Ensure user_id is set
        bool success = m_database->update_event(event);
        
        if (success) {
            // SHARED CALENDAR: Broadcast update to ALL connected users
            broadcast_event_update(event, "updated");
            std::cout << "Event updated and broadcast to all users: " << event.title << " (Updated by User: " << user_id << ")" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error updating event: " << e.what() << std::endl;
    }
}



// void EventServer::handle_event_delete(std::shared_ptr<WebSocketSession> session, const nlohmann::json& data) {
//     if (!is_authenticated(session, data)) return;
    
//     try {
//         int event_id = data["id"];
//         std::string token = data["auth_token"];
//         int user_id = m_authManager->get_user_id_by_token(token);
        
//         // Verify user owns this event
//         Event existing_event = m_database->get_event_by_id(event_id);
//         if (existing_event.user_id != user_id) {
//             nlohmann::json error_response = {
//                 {"error", "You can only delete your own events"},
//                 {"code", "PERMISSION_DENIED"}
//             };
//             auto message = Protocol::create_message(Protocol::AUTH_ERROR, error_response);
//             session->send(message.dump());
//             return;
//         }
        
//         bool success = m_database->delete_event(event_id);
        
//         if (success) {
//             nlohmann::json delete_data = {{"id", event_id}};
//             auto message = Protocol::create_message(Protocol::EVENT_DELETE, delete_data);
//             broadcast_to_all(message.dump());
//             std::cout << "Event deleted: " << event_id << " (User: " << user_id << ")" << std::endl;
//         }
        
//     } catch (const std::exception& e) {
//         std::cerr << "Error deleting event: " << e.what() << std::endl;
//     }
// }

void EventServer::handle_event_delete(std::shared_ptr<WebSocketSession> session, const nlohmann::json& data) {
    if (!is_authenticated(session, data)) return;
    
    try {
        int event_id = data["id"];
        std::string token = data["auth_token"];
        int user_id = m_authManager->get_user_id_by_token(token);
        
        // OPTIONAL: Check if user owns this event (for delete permissions)
        Event existing_event = m_database->get_event_by_id(event_id);
        if (existing_event.user_id != user_id) {
            nlohmann::json error_response = {
                {"error", "You can only delete your own events"},
                {"code", "PERMISSION_DENIED"}
            };
            auto message = Protocol::create_message(Protocol::AUTH_ERROR, error_response);
            session->send(message.dump());
            return;
        }
        
        bool success = m_database->delete_event(event_id);
        
        if (success) {
            // SHARED CALENDAR: Broadcast deletion to ALL connected users
            nlohmann::json delete_data = {{"id", event_id}};
            auto message = Protocol::create_message(Protocol::EVENT_DELETE, delete_data);
            broadcast_to_all(message.dump());
            std::cout << "Event deleted and broadcast to all users: " << event_id << " (Deleted by User: " << user_id << ")" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error deleting event: " << e.what() << std::endl;
    }
}


// void EventServer::handle_event_list(std::shared_ptr<WebSocketSession> session, const nlohmann::json& data) {
//     if (!is_authenticated(session, data)) return;
    
//     try {
//         std::string token = data["auth_token"];
//         int user_id = m_authManager->get_user_id_by_token(token);
        
//         // Only return events for this user
//         auto events = m_database->get_events_for_user(user_id);
//         nlohmann::json events_json = nlohmann::json::array();
        
//         for (const auto& event : events) {
//             events_json.push_back(event.to_json());
//         }
        
//         auto message = Protocol::create_message(Protocol::EVENT_LIST, events_json);
//         session->send(message.dump());
        
//     } catch (const std::exception& e) {
//         std::cerr << "Error listing events: " << e.what() << std::endl;
//     }
// }

void EventServer::handle_event_list(std::shared_ptr<WebSocketSession> session, const nlohmann::json& data) {
    if (!is_authenticated(session, data)) return;
    
    try {
        // SHARED CALENDAR: Show ALL events to authenticated users
        auto events = m_database->get_all_events();
        nlohmann::json events_json = nlohmann::json::array();
        
        for (const auto& event : events) {
            events_json.push_back(event.to_json());
        }
        
        auto message = Protocol::create_message(Protocol::EVENT_LIST, events_json);
        session->send(message.dump());
        
        std::cout << "Sent " << events.size() << " events to authenticated user" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error listing events: " << e.what() << std::endl;
    }
}

void EventServer::broadcast_to_all(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_sessions_lock);
    
    for (auto& session : m_sessions) {
        try {
            session->send(message);
        } catch (const std::exception& e) {
            std::cerr << "Error broadcasting message: " << e.what() << std::endl;
        }
    }
}

void EventServer::broadcast_event_update(const Event& event, const std::string& action) {
    nlohmann::json data = event.to_json();
    data["action"] = action;
    
    auto message = Protocol::create_message(Protocol::EVENT_UPDATE, data);
    broadcast_to_all(message.dump());
}

// void EventServer::send_reminder(const Event& event) {
//     nlohmann::json reminder_data = event.to_json();
//     reminder_data["message"] = "Reminder: " + event.title + " starts in " + 
//                              std::to_string(event.time_until_event().count()) + " minutes";
    
//     auto message = Protocol::create_message(Protocol::REMINDER, reminder_data);
//     broadcast_to_all(message.dump());
// }

void EventServer::send_reminder(const Event& event) {
    nlohmann::json reminder_data = event.to_json();
    reminder_data["message"] = "Reminder: " + event.title + " starts in " + 
                             std::to_string(event.time_until_event().count()) + " minutes";
    
    auto message = Protocol::create_message(Protocol::REMINDER, reminder_data);
    
    // SHARED REMINDERS: Send reminder to ALL authenticated users
    broadcast_to_all(message.dump());
    std::cout << "Reminder sent to all users for event: " << event.title << std::endl;
}

// Authentication methods
bool EventServer::is_authenticated(std::shared_ptr<WebSocketSession> session, const nlohmann::json& data) {
    if (!data.contains("auth_token")) {
        nlohmann::json error_response = {
            {"error", "Authentication required"},
            {"code", "AUTH_REQUIRED"}
        };
        auto message = Protocol::create_message(Protocol::AUTH_ERROR, error_response);
        session->send(message.dump());
        return false;
    }
    
    std::string token = data["auth_token"];
    if (!m_authManager->validate_token(token)) {
        nlohmann::json error_response = {
            {"error", "Invalid or expired token"},
            {"code", "INVALID_TOKEN"}
        };
        auto message = Protocol::create_message(Protocol::AUTH_ERROR, error_response);
        session->send(message.dump());
        return false;
    }
    
    return true;
}

void EventServer::handle_auth_login(std::shared_ptr<WebSocketSession> session, const nlohmann::json& data) {
    try {
        std::string username = data["username"];
        std::string password = data["password"];
        
        AuthToken token = m_authManager->login(username, password);
        
        if (token.token.empty()) {
            nlohmann::json error_response = {
                {"error", "Invalid username or password"},
                {"code", "INVALID_CREDENTIALS"}
            };
            auto message = Protocol::create_message(Protocol::AUTH_ERROR, error_response);
            session->send(message.dump());
            return;
        }
        
        // Get user info
        User user = m_authManager->get_user_by_token(token.token);
        
        nlohmann::json success_response = {
            {"token", token.token},
            {"user", user.to_json()}
        };
        auto message = Protocol::create_message(Protocol::AUTH_SUCCESS, success_response);
        session->send(message.dump());
        
        std::cout << "User " << username << " logged in successfully" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Login error: " << e.what() << std::endl;
        nlohmann::json error_response = {
            {"error", "Login failed"},
            {"code", "LOGIN_ERROR"}
        };
        auto message = Protocol::create_message(Protocol::AUTH_ERROR, error_response);
        session->send(message.dump());
    }
}

void EventServer::handle_auth_register(std::shared_ptr<WebSocketSession> session, const nlohmann::json& data) {
    try {
        std::string username = data["username"];
        std::string email = data["email"];
        std::string password = data["password"];
        std::string display_name = data.contains("display_name") ? data["display_name"].get<std::string>() : username;
        
        bool success = m_authManager->register_user(username, email, password, display_name);
        
        if (success) {
            nlohmann::json success_response = {
                {"message", "User registered successfully"}
            };
            auto message = Protocol::create_message(Protocol::AUTH_SUCCESS, success_response);
            session->send(message.dump());
            
            std::cout << "User " << username << " registered successfully" << std::endl;
        } else {
            nlohmann::json error_response = {
                {"error", "Registration failed. Username or email may already exist."},
                {"code", "REGISTRATION_FAILED"}
            };
            auto message = Protocol::create_message(Protocol::AUTH_ERROR, error_response);
            session->send(message.dump());
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Registration error: " << e.what() << std::endl;
        nlohmann::json error_response = {
            {"error", "Registration failed"},
            {"code", "REGISTRATION_ERROR"}
        };
        auto message = Protocol::create_message(Protocol::AUTH_ERROR, error_response);
        session->send(message.dump());
    }
}

void EventServer::handle_auth_logout(std::shared_ptr<WebSocketSession> session, const nlohmann::json& data) {
    try {
        if (data.contains("auth_token")) {
            std::string token = data["auth_token"];
            m_authManager->logout(token);
        }
        
        nlohmann::json success_response = {
            {"message", "Logged out successfully"}
        };
        auto message = Protocol::create_message(Protocol::AUTH_SUCCESS, success_response);
        session->send(message.dump());
        
    } catch (const std::exception& e) {
        std::cerr << "Logout error: " << e.what() << std::endl;
    }
}