#ifndef EVENT_SERVER_H
#define EVENT_SERVER_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <set>
#include "Database.h"
#include "ReminderManager.h"
#include "Event.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
public:
    explicit WebSocketSession(tcp::socket&& socket);
    ~WebSocketSession();
    
    void run();
    void send(const std::string& message);
    void close();
    
    void set_message_handler(std::function<void(std::shared_ptr<WebSocketSession>, const std::string&)> handler);
    void set_close_handler(std::function<void(std::shared_ptr<WebSocketSession>)> handler);
    void set_connect_handler(std::function<void(std::shared_ptr<WebSocketSession>)> handler);

private:
    void on_accept(beast::error_code ec);
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);

    websocket::stream<beast::tcp_stream> m_ws;
    beast::flat_buffer m_buffer;
    std::vector<std::shared_ptr<std::string const>> m_queue;
    
    std::function<void(std::shared_ptr<WebSocketSession>, const std::string&)> m_message_handler;
    std::function<void(std::shared_ptr<WebSocketSession>)> m_close_handler;
    std::function<void(std::shared_ptr<WebSocketSession>)> m_connect_handler;
};

class EventServer {
public:
    EventServer();
    ~EventServer();
    
    void start(int port = 8080);
    void stop();
    
private:
    void do_accept();
    void on_accept(beast::error_code ec, tcp::socket socket);
    
    void on_message(std::shared_ptr<WebSocketSession> session, const std::string& message);
    void on_session_close(std::shared_ptr<WebSocketSession> session);
    void on_connection_established(std::shared_ptr<WebSocketSession> session);
    
    // Message handlers
    void handle_event_create(std::shared_ptr<WebSocketSession> session, const nlohmann::json& data);
    void handle_event_update(std::shared_ptr<WebSocketSession> session, const nlohmann::json& data);
    void handle_event_delete(std::shared_ptr<WebSocketSession> session, const nlohmann::json& data);
    void handle_event_list(std::shared_ptr<WebSocketSession> session, const nlohmann::json& data);
    
    // Broadcast functions
    void broadcast_to_all(const std::string& message);
    void broadcast_event_update(const Event& event, const std::string& action);
    void send_reminder(const Event& event);

    net::io_context m_ioc;
    tcp::acceptor m_acceptor;
    std::unique_ptr<Database> m_database;
    std::unique_ptr<ReminderManager> m_reminderManager;
    std::thread m_thread;
    std::mutex m_sessions_lock;
    std::set<std::shared_ptr<WebSocketSession>> m_sessions;
    bool m_running;
};

#endif // EVENT_SERVER_H