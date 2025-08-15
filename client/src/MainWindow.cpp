#include "MainWindow.h"
#include "EventDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QHeaderView>
#include <QMessageBox>
#include <QCloseEvent>
#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_isConnected(false)
{
    setupUI();
    setupSystemTray();
    
    // Initialize business logic
    m_client = std::make_unique<WebSocketClient>(this);
    m_eventModel = std::make_unique<EventModel>(this);
    
    // Set model to table view
    m_eventTable->setModel(m_eventModel.get());
    
    // Configure table view
    m_eventTable->horizontalHeader()->setStretchLastSection(true);
    m_eventTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_eventTable->setAlternatingRowColors(true);
    
    // Connect WebSocket client signals
    connect(m_client.get(), &WebSocketClient::connected,
            this, &MainWindow::onConnectedToServer);
    connect(m_client.get(), &WebSocketClient::disconnected,
            this, &MainWindow::onDisconnectedFromServer);
    connect(m_client.get(), &WebSocketClient::eventReceived,
            this, &MainWindow::onEventReceived);
    connect(m_client.get(), &WebSocketClient::eventListReceived,
            this, &MainWindow::onEventListReceived);
    connect(m_client.get(), &WebSocketClient::reminderReceived,
            this, &MainWindow::onReminderReceived);
    connect(m_client.get(), &WebSocketClient::errorOccurred,
            this, &MainWindow::onConnectionError);
    
    // Setup status timer
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &MainWindow::updateConnectionStatus);
    m_statusTimer->start(1000); // Update every second
    
    updateButtons();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI() {
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    
    // Connection section
    m_connectionLayout = new QHBoxLayout();
    m_serverAddressEdit = new QLineEdit("ws://localhost:8080");
    m_connectButton = new QPushButton("Connect");
    m_disconnectButton = new QPushButton("Disconnect");
    m_statusLabel = new QLabel("Disconnected");
    
    m_connectionLayout->addWidget(new QLabel("Server:"));
    m_connectionLayout->addWidget(m_serverAddressEdit);
    m_connectionLayout->addWidget(m_connectButton);
    m_connectionLayout->addWidget(m_disconnectButton);
    m_connectionLayout->addStretch();
    m_connectionLayout->addWidget(m_statusLabel);
    
    // Event table
    m_eventTable = new QTableView();
    
    // Button section
    m_buttonLayout = new QHBoxLayout();
    m_addButton = new QPushButton("Add Event");
    m_editButton = new QPushButton("Edit Event");
    m_deleteButton = new QPushButton("Delete Event");
    m_refreshButton = new QPushButton("Refresh");
    
    m_buttonLayout->addWidget(m_addButton);
    m_buttonLayout->addWidget(m_editButton);
    m_buttonLayout->addWidget(m_deleteButton);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_refreshButton);
    
    // Add layouts to main layout
    m_mainLayout->addLayout(m_connectionLayout);
    m_mainLayout->addWidget(m_eventTable);
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Connect button signals
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_addButton, &QPushButton::clicked, this, &MainWindow::onAddEventClicked);
    connect(m_editButton, &QPushButton::clicked, this, &MainWindow::onEditEventClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteEventClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
    
    // Table double-click
    connect(m_eventTable, &QTableView::doubleClicked, this, &MainWindow::onEventDoubleClicked);
    
    // Window properties
    setWindowTitle("Event Manager Client");
    resize(800, 600);
}

void MainWindow::setupSystemTray() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }
    
    // Create system tray icon
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    m_trayIcon->setToolTip("Event Manager");
    
    // Create tray menu
    m_trayMenu = new QMenu(this);
    
    QAction* showAction = m_trayMenu->addAction("Show");
    connect(showAction, &QAction::triggered, this, &QWidget::show);
    
    QAction* hideAction = m_trayMenu->addAction("Hide");
    connect(hideAction, &QAction::triggered, this, &QWidget::hide);
    
    m_trayMenu->addSeparator();
    
    QAction* quitAction = m_trayMenu->addAction("Quit");
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    
    m_trayIcon->setContextMenu(m_trayMenu);
    
    // Connect tray icon signals
    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &MainWindow::onTrayIconActivated);
    
    m_trayIcon->show();
}

void MainWindow::onConnectedToServer() {
    m_isConnected = true;
    m_statusLabel->setText("Connected");
    m_statusLabel->setStyleSheet("color: green;");
    updateButtons();
    
    // Request all events
    m_client->requestEventList();
}

void MainWindow::onDisconnectedFromServer() {
    m_isConnected = false;
    m_statusLabel->setText("Disconnected");
    m_statusLabel->setStyleSheet("color: red;");
    updateButtons();
}

void MainWindow::onEventReceived(const Event& event, const QString& action) {
    if (action == "created" || action == "updated") {
        m_eventModel->updateEvent(event);
    } else if (action == "deleted") {
        m_eventModel->removeEvent(event.id);
    }
}

void MainWindow::onEventListReceived(const std::vector<Event>& events) {
    m_eventModel->setEvents(events);
}

void MainWindow::onReminderReceived(const Event& event, const QString& message) {
    showReminder(QString::fromStdString(event.title), message);
}

void MainWindow::onConnectionError(const QString& error) {
    QMessageBox::warning(this, "Connection Error", error);
    m_isConnected = false;
    updateButtons();
}

void MainWindow::onAddEventClicked() {
    EventDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Event event = dialog.getEvent();
        m_client->createEvent(event);
    }
}

void MainWindow::onEditEventClicked() {
    QModelIndexList selection = m_eventTable->selectionModel()->selectedRows();
    if (selection.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select an event to edit.");
        return;
    }
    
    int row = selection.first().row();
    Event event = m_eventModel->getEvent(row);
    
    EventDialog dialog(event, this);
    if (dialog.exec() == QDialog::Accepted) {
        Event updatedEvent = dialog.getEvent();
        m_client->updateEvent(updatedEvent);
    }
}

void MainWindow::onDeleteEventClicked() {
    QModelIndexList selection = m_eventTable->selectionModel()->selectedRows();
    if (selection.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select an event to delete.");
        return;
    }
    
    int row = selection.first().row();
    Event event = m_eventModel->getEvent(row);
    
    int ret = QMessageBox::question(this, "Confirm Delete", 
                                  QString("Are you sure you want to delete '%1'?")
                                  .arg(QString::fromStdString(event.title)));
    
    if (ret == QMessageBox::Yes) {
        m_client->deleteEvent(event.id);
    }
}

void MainWindow::onRefreshClicked() {
    if (m_isConnected) {
        m_client->requestEventList();
    }
}

void MainWindow::onConnectClicked() {
    QString address = m_serverAddressEdit->text().trimmed();
    if (address.isEmpty()) {
        QMessageBox::warning(this, "Invalid Address", "Please enter a server address.");
        return;
    }
    
    m_currentServerAddress = address;
    m_client->connectToServer(address);
}

void MainWindow::onDisconnectClicked() {
    m_client->disconnectFromServer();
}

void MainWindow::onEventDoubleClicked(const QModelIndex& index) {
    if (index.isValid()) {
        onEditEventClicked();
    }
}

void MainWindow::updateConnectionStatus() {
    // Update status display with additional info if needed
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        if (isVisible()) {
            hide();
        } else {
            show();
            raise();
            activateWindow();
        }
        break;
    default:
        break;
    }
}

void MainWindow::showReminder(const QString& title, const QString& message) {
    if (m_trayIcon && m_trayIcon->isVisible()) {
        m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 5000);
    } else {
        QMessageBox::information(this, title, message);
    }
}

void MainWindow::updateButtons() {
    m_connectButton->setEnabled(!m_isConnected);
    m_disconnectButton->setEnabled(m_isConnected);
    m_addButton->setEnabled(m_isConnected);
    m_editButton->setEnabled(m_isConnected);
    m_deleteButton->setEnabled(m_isConnected);
    m_refreshButton->setEnabled(m_isConnected);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}