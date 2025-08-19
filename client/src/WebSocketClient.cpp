#include "WebSocketClient.h"
#include "Protocol.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <nlohmann/json.hpp>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
    , m_isConnected(false)
    , m_isAuthenticated(false)
{
    m_webSocket = std::make_unique<QWebSocket>();
    
    // Setup heartbeat timer
    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &WebSocketClient::sendHeartbeat);
    
    // Connect WebSocket signals
    connect(m_webSocket.get(), &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(m_webSocket.get(), &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(m_webSocket.get(), &QWebSocket::textMessageReceived, 
            this, &WebSocketClient::onTextMessageReceived);
    connect(m_webSocket.get(), &QWebSocket::errorOccurred,
            this, &WebSocketClient::onError);
}

WebSocketClient::~WebSocketClient() {
    disconnectFromServer();
}

void WebSocketClient::connectToServer(const QString& url) {
    if (m_isConnected) {
        disconnectFromServer();
    }
    
    m_serverUrl = url;
    qDebug() << "Connecting to server:" << url;
    m_webSocket->open(QUrl(url));
}

void WebSocketClient::disconnectFromServer() {
    if (m_isConnected) {
        m_heartbeatTimer->stop();
        m_webSocket->close();
    }
}

bool WebSocketClient::isConnected() const {
    return m_isConnected;
}

void WebSocketClient::createEvent(const Event& event) {
    if (!m_isConnected || !m_isAuthenticated) return;
    
    auto json_data = event.to_json();
    json_data["auth_token"] = m_authToken.toStdString();
    sendMessage(QString::fromStdString(Protocol::EVENT_CREATE), json_data);
}

void WebSocketClient::updateEvent(const Event& event) {
    if (!m_isConnected || !m_isAuthenticated) return;
    
    auto json_data = event.to_json();
    json_data["auth_token"] = m_authToken.toStdString();
    sendMessage(QString::fromStdString(Protocol::EVENT_UPDATE), json_data);
}

void WebSocketClient::deleteEvent(int eventId) {
    if (!m_isConnected || !m_isAuthenticated) return;
    
    nlohmann::json data = {
        {"id", eventId},
        {"auth_token", m_authToken.toStdString()}
    };
    sendMessage(QString::fromStdString(Protocol::EVENT_DELETE), data);
}

void WebSocketClient::requestEventList() {
    if (!m_isConnected || !m_isAuthenticated) return;
    
    nlohmann::json data = {
        {"auth_token", m_authToken.toStdString()}
    };
    sendMessage(QString::fromStdString(Protocol::EVENT_LIST), data);
}

void WebSocketClient::login(const QString& username, const QString& password) {
    if (!m_isConnected) return;
    
    nlohmann::json login_data = {
        {"username", username.toStdString()},
        {"password", password.toStdString()}
    };
    sendMessage(QString::fromStdString(Protocol::AUTH_LOGIN), login_data);
}

void WebSocketClient::registerUser(const QString& username, const QString& email, 
                                 const QString& password, const QString& displayName) {
    if (!m_isConnected) return;
    
    nlohmann::json register_data = {
        {"username", username.toStdString()},
        {"email", email.toStdString()},
        {"password", password.toStdString()},
        {"display_name", displayName.toStdString()}
    };
    sendMessage(QString::fromStdString(Protocol::AUTH_REGISTER), register_data);
}

void WebSocketClient::logout() {
    if (!m_isConnected || !m_isAuthenticated) return;
    
    nlohmann::json logout_data = {
        {"auth_token", m_authToken.toStdString()}
    };
    sendMessage(QString::fromStdString(Protocol::AUTH_LOGOUT), logout_data);
    
    // Clear local auth state
    m_authToken.clear();
    m_currentUser.clear();
    m_isAuthenticated = false;
}

void WebSocketClient::onConnected() {
    m_isConnected = true;
    qDebug() << "Connected to server";
    
    // Start heartbeat
    m_heartbeatTimer->start(30000); // 30 seconds
    
    emit connected();
}

void WebSocketClient::onDisconnected() {
    m_isConnected = false;
    m_heartbeatTimer->stop();
    qDebug() << "Disconnected from server";
    
    emit disconnected();
}

void WebSocketClient::onTextMessageReceived(const QString& message) {
    try {
        auto [type, data] = Protocol::parse_message(message.toStdString());
        handleMessage(QString::fromStdString(type), data);
    } catch (const std::exception& e) {
        qDebug() << "Error parsing message:" << e.what();
        emit errorOccurred(QString("Failed to parse server message: %1").arg(e.what()));
    }
}

void WebSocketClient::onError(QAbstractSocket::SocketError error) {
    QString errorString;
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
        errorString = "Connection refused. Make sure the server is running.";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        errorString = "Server closed the connection.";
        break;
    case QAbstractSocket::HostNotFoundError:
        errorString = "Server host not found.";
        break;
    case QAbstractSocket::SocketTimeoutError:
        errorString = "Connection timeout.";
        break;
    default:
        errorString = QString("Network error: %1").arg(m_webSocket->errorString());
        break;
    }
    
    qDebug() << "WebSocket error:" << errorString;
    emit errorOccurred(errorString);
}

void WebSocketClient::sendHeartbeat() {
    if (m_isConnected) {
        sendMessage(QString::fromStdString(Protocol::HEARTBEAT));
    }
}

void WebSocketClient::sendMessage(const QString& type, const nlohmann::json& data) {
    if (!m_isConnected) return;
    
    try {
        auto message = Protocol::create_message(type.toStdString(), data);
        QString messageStr = QString::fromStdString(message.dump());
        m_webSocket->sendTextMessage(messageStr);
    } catch (const std::exception& e) {
        qDebug() << "Error sending message:" << e.what();
        emit errorOccurred(QString("Failed to send message: %1").arg(e.what()));
    }
}

// void WebSocketClient::handleMessage(const QString& type, const nlohmann::json& data) {
//     try {
//         if (type == QString::fromStdString(Protocol::AUTH_SUCCESS)) {
//             if (data.contains("token")) {
//                 // Login successful
//                 m_authToken = QString::fromStdString(data["token"]);
//                 m_currentUser = QString::fromStdString(data["user"]["username"]);
//                 m_isAuthenticated = true;
//                 emit authenticationSucceeded(m_currentUser, m_authToken);
//             } else {
//                 // Registration successful
//                 emit registrationSucceeded();
//             }
            
//         } else if (type == QString::fromStdString(Protocol::AUTH_ERROR)) {
//             QString error = QString::fromStdString(data["error"]);
//             QString code = data.contains("code") ? QString::fromStdString(data["code"]) : "";
            
//             if (code == "REGISTRATION_FAILED" || code == "REGISTRATION_ERROR") {
//                 emit registrationFailed(error);
//             } else {
//                 emit authenticationFailed(error);
//             }
            
//         } else if (type == QString::fromStdString(Protocol::EVENT_LIST)) {
//             std::vector<Event> events;
//             for (const auto& eventJson : data) {
//                 events.push_back(Event::from_json(eventJson));
//             }
//             emit eventListReceived(events);
            
//         } else if (type == QString::fromStdString(Protocol::EVENT_UPDATE)) {
//             Event event = Event::from_json(data);
//             QString action = QString::fromStdString(data["action"]);
//             emit eventReceived(event, action);
            
//         } else if (type == QString::fromStdString(Protocol::EVENT_DELETE)) {
//             Event event;
//             event.id = data["id"];
//             emit eventReceived(event, "deleted");
            
//         } else if (type == QString::fromStdString(Protocol::REMINDER)) {
//             Event event = Event::from_json(data);
//             QString message = QString::fromStdString(data["message"]);
//             emit reminderReceived(event, message);
            
//         } else if (type == QString::fromStdString(Protocol::HEARTBEAT)) {
//             // Heartbeat response - no action needed
//         }
        
//     } catch (const std::exception& e) {
//         qDebug() << "Error handling message:" << e.what();
//         emit errorOccurred(QString("Failed to handle server message: %1").arg(e.what()));
//     }
// }

void WebSocketClient::handleMessage(const QString& type, const nlohmann::json& data) {
    try {
        qDebug() << "🔔 CLIENT: Received message type:" << type;
        
        if (type == QString::fromStdString(Protocol::AUTH_SUCCESS)) {
            if (data.contains("token")) {
                // Login successful
                m_authToken = QString::fromStdString(data["token"]);
                m_currentUser = QString::fromStdString(data["user"]["username"]);
                m_isAuthenticated = true;
                emit authenticationSucceeded(m_currentUser, m_authToken);
            } else {
                // Registration successful
                emit registrationSucceeded();
            }
            
        } else if (type == QString::fromStdString(Protocol::AUTH_ERROR)) {
            QString error = QString::fromStdString(data["error"]);
            QString code = data.contains("code") ? QString::fromStdString(data["code"]) : "";
            
            if (code == "REGISTRATION_FAILED" || code == "REGISTRATION_ERROR") {
                emit registrationFailed(error);
            } else {
                emit authenticationFailed(error);
            }
            
        } else if (type == QString::fromStdString(Protocol::EVENT_LIST)) {
            std::vector<Event> events;
            for (const auto& eventJson : data) {
                events.push_back(Event::from_json(eventJson));
            }
            emit eventListReceived(events);
            
        } else if (type == QString::fromStdString(Protocol::EVENT_UPDATE)) {
            Event event = Event::from_json(data);
            QString action = QString::fromStdString(data["action"]);
            emit eventReceived(event, action);
            
        } else if (type == QString::fromStdString(Protocol::EVENT_DELETE)) {
            Event event;
            event.id = data["id"];
            emit eventReceived(event, "deleted");
            
        } else if (type == QString::fromStdString(Protocol::REMINDER)) {
            qDebug() << "🔔 CLIENT: Processing REMINDER message";
            
            // Parse the reminder data
            Event event = Event::from_json(data);
            QString message;
            
            if (data.contains("message")) {
                message = QString::fromStdString(data["message"]);
            } else {
                // Fallback message if server doesn't send one
                message = QString("Reminder: %1 is starting soon!").arg(QString::fromStdString(event.title));
            }
            
            qDebug() << "🔔 CLIENT: Emitting reminderReceived signal for event:" << QString::fromStdString(event.title);
            qDebug() << "🔔 CLIENT: Reminder message:" << message;
            
            emit reminderReceived(event, message);
            
        } else if (type == QString::fromStdString(Protocol::HEARTBEAT)) {
            // Heartbeat response - no action needed
            qDebug() << "CLIENT: Heartbeat received";
        } else {
            qDebug() << "CLIENT: Unknown message type:" << type;
        }
        
    } catch (const std::exception& e) {
        qDebug() << "Error handling message:" << e.what();
        emit errorOccurred(QString("Failed to handle server message: %1").arg(e.what()));
    }
}