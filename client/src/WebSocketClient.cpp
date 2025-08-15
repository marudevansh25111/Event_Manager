#include "WebSocketClient.h"
#include "Protocol.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <nlohmann/json.hpp>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
    , m_isConnected(false)
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
    if (!m_isConnected) return;
    
    auto json_data = event.to_json();
    sendMessage(QString::fromStdString(Protocol::EVENT_CREATE), json_data);
}

void WebSocketClient::updateEvent(const Event& event) {
    if (!m_isConnected) return;
    
    auto json_data = event.to_json();
    sendMessage(QString::fromStdString(Protocol::EVENT_UPDATE), json_data);
}

void WebSocketClient::deleteEvent(int eventId) {
    if (!m_isConnected) return;
    
    nlohmann::json data = {{"id", eventId}};
    sendMessage(QString::fromStdString(Protocol::EVENT_DELETE), data);
}

void WebSocketClient::requestEventList() {
    if (!m_isConnected) return;
    
    sendMessage(QString::fromStdString(Protocol::EVENT_LIST));
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

void WebSocketClient::handleMessage(const QString& type, const nlohmann::json& data) {
    try {
        if (type == QString::fromStdString(Protocol::EVENT_LIST)) {
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
            Event event = Event::from_json(data);
            QString message = QString::fromStdString(data["message"]);
            emit reminderReceived(event, message);
            
        } else if (type == QString::fromStdString(Protocol::HEARTBEAT)) {
            // Heartbeat response - no action needed
        }
        
    } catch (const std::exception& e) {
        qDebug() << "Error handling message:" << e.what();
        emit errorOccurred(QString("Failed to handle server message: %1").arg(e.what()));
    }
}