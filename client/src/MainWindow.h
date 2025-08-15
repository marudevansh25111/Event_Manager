#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <memory>
#include "WebSocketClient.h"
#include "EventModel.h"

QT_BEGIN_NAMESPACE
class QTableView;
class QPushButton;
class QLabel;
class QLineEdit;
class QVBoxLayout;
class QHBoxLayout;
QT_END_NAMESPACE

class EventDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectedToServer();
    void onDisconnectedFromServer();
    void onEventReceived(const Event& event, const QString& action);
    void onEventListReceived(const std::vector<Event>& events);
    void onReminderReceived(const Event& event, const QString& message);
    void onConnectionError(const QString& error);
    
    void onAddEventClicked();
    void onEditEventClicked();
    void onDeleteEventClicked();
    void onRefreshClicked();
    void onConnectClicked();
    void onDisconnectClicked();
    
    void onEventDoubleClicked(const QModelIndex& index);
    void updateConnectionStatus();
    
    // System tray
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void showReminder(const QString& title, const QString& message);

private:
    void setupUI();
    void setupSystemTray();
    void connectToServer();
    void updateButtons();
    void closeEvent(QCloseEvent* event) override;
    
    // UI components
    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_connectionLayout;
    QHBoxLayout* m_buttonLayout;
    
    QLineEdit* m_serverAddressEdit;
    QPushButton* m_connectButton;
    QPushButton* m_disconnectButton;
    QLabel* m_statusLabel;
    
    QTableView* m_eventTable;
    QPushButton* m_addButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
    QPushButton* m_refreshButton;
    
    // System tray
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
    
    // Business logic
    std::unique_ptr<WebSocketClient> m_client;
    std::unique_ptr<EventModel> m_eventModel;
    QTimer* m_statusTimer;
    
    QString m_currentServerAddress;
    bool m_isConnected;
};

#endif // MAINWINDOW_H