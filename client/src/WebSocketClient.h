#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <memory>
#include "Event.h"

class WebSocketClient : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketClient(QObject *parent = nullptr);
    ~WebSocketClient();

    void connectToServer(const QString& url);
    void disconnectFromServer();
    bool isConnected() const;

    // Event operations
    void createEvent(const Event& event);
    void updateEvent(const Event& event);
    void deleteEvent(int eventId);
    void requestEventList();

signals:
    void connected();
    void disconnected();
    void eventReceived(const Event& event, const QString& action);
    void eventListReceived(const std::vector<Event>& events);
    void reminderReceived(const Event& event, const QString& message);
    void errorOccurred(const QString& error);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onError(QAbstractSocket::SocketError error);
    void sendHeartbeat();

private:
    void sendMessage(const QString& type, const nlohmann::json& data = {});
    void handleMessage(const QString& type, const nlohmann::json& data);

    std::unique_ptr<QWebSocket> m_webSocket;
    QTimer* m_heartbeatTimer;
    QString m_serverUrl;
    bool m_isConnected;
};

#endif // WEBSOCKETCLIENT_H