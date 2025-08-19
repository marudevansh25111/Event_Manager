#include "LoginDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent), m_isRegistering(false)
{
    setupUI();
    setWindowTitle("Login to Event Manager");
    resize(350, 280);
}

void LoginDialog::setupUI() {
    setModal(true);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget();
    
    setupLoginTab();
    setupRegisterTab();
    
    m_tabWidget->addTab(m_loginTab, "Login");
    m_tabWidget->addTab(m_registerTab, "Register");
    
    // Status label
    m_statusLabel = new QLabel();
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
    m_statusLabel->hide();
    
    mainLayout->addWidget(m_tabWidget);
    mainLayout->addWidget(m_statusLabel);
    
    // Connect tab change signal
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &LoginDialog::onTabChanged);
    
    // Initial validation
    validateInput();
}

void LoginDialog::setupLoginTab() {
    m_loginTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_loginTab);
    
    // Form layout
    QFormLayout* formLayout = new QFormLayout();
    
    m_loginUsernameEdit = new QLineEdit();
    m_loginUsernameEdit->setPlaceholderText("Enter your username");
    
    m_loginPasswordEdit = new QLineEdit();
    m_loginPasswordEdit->setEchoMode(QLineEdit::Password);
    m_loginPasswordEdit->setPlaceholderText("Enter your password");
    
    formLayout->addRow("Username:", m_loginUsernameEdit);
    formLayout->addRow("Password:", m_loginPasswordEdit);
    
    layout->addLayout(formLayout);
    layout->addStretch();
    
    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_loginButton = new QPushButton("Login");
    m_loginButton->setDefault(true);
    m_loginButton->setEnabled(false);
    
    QPushButton* cancelButton = new QPushButton("Cancel");
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_loginButton);
    buttonLayout->addWidget(cancelButton);
    
    layout->addLayout(buttonLayout);
    
    // Connect signals
    connect(m_loginUsernameEdit, &QLineEdit::textChanged, this, &LoginDialog::validateInput);
    connect(m_loginPasswordEdit, &QLineEdit::textChanged, this, &LoginDialog::validateInput);
    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    // Enter key handling
    connect(m_loginPasswordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
}

void LoginDialog::setupRegisterTab() {
    m_registerTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_registerTab);
    
    // Form layout
    QFormLayout* formLayout = new QFormLayout();
    
    m_registerUsernameEdit = new QLineEdit();
    m_registerUsernameEdit->setPlaceholderText("3-20 characters, alphanumeric + underscore");
    
    m_registerEmailEdit = new QLineEdit();
    m_registerEmailEdit->setPlaceholderText("your.email@example.com");
    
    m_registerPasswordEdit = new QLineEdit();
    m_registerPasswordEdit->setEchoMode(QLineEdit::Password);
    m_registerPasswordEdit->setPlaceholderText("Minimum 6 characters");
    
    m_registerDisplayNameEdit = new QLineEdit();
    m_registerDisplayNameEdit->setPlaceholderText("Your display name (optional)");
    
    formLayout->addRow("Username*:", m_registerUsernameEdit);
    formLayout->addRow("Email*:", m_registerEmailEdit);
    formLayout->addRow("Password*:", m_registerPasswordEdit);
    formLayout->addRow("Display Name:", m_registerDisplayNameEdit);
    
    layout->addLayout(formLayout);
    layout->addStretch();
    
    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_registerButton = new QPushButton("Register");
    m_registerButton->setDefault(true);
    m_registerButton->setEnabled(false);
    
    QPushButton* cancelButton = new QPushButton("Cancel");
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_registerButton);
    buttonLayout->addWidget(cancelButton);
    
    layout->addLayout(buttonLayout);
    
    // Connect signals
    connect(m_registerUsernameEdit, &QLineEdit::textChanged, this, &LoginDialog::validateInput);
    connect(m_registerEmailEdit, &QLineEdit::textChanged, this, &LoginDialog::validateInput);
    connect(m_registerPasswordEdit, &QLineEdit::textChanged, this, &LoginDialog::validateInput);
    connect(m_registerDisplayNameEdit, &QLineEdit::textChanged, this, &LoginDialog::validateInput);
    connect(m_registerButton, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    // Enter key handling
    connect(m_registerDisplayNameEdit, &QLineEdit::returnPressed, this, &LoginDialog::onRegisterClicked);
}

QString LoginDialog::getUsername() const {
    return m_isRegistering ? m_registerUsernameEdit->text().trimmed() 
                           : m_loginUsernameEdit->text().trimmed();
}

QString LoginDialog::getPassword() const {
    return m_isRegistering ? m_registerPasswordEdit->text() 
                           : m_loginPasswordEdit->text();
}

QString LoginDialog::getEmail() const {
    return m_registerEmailEdit->text().trimmed();
}

QString LoginDialog::getDisplayName() const {
    QString displayName = m_registerDisplayNameEdit->text().trimmed();
    return displayName.isEmpty() ? getUsername() : displayName;
}

bool LoginDialog::isRegistering() const {
    return m_isRegistering;
}

void LoginDialog::onLoginClicked() {
    if (isValidLoginInput()) {
        m_isRegistering = false;
        accept();
    }
}

void LoginDialog::onRegisterClicked() {
    if (isValidRegisterInput()) {
        m_isRegistering = true;
        accept();
    }
}

void LoginDialog::validateInput() {
    if (m_tabWidget->currentIndex() == 0) {
        // Login tab
        m_loginButton->setEnabled(isValidLoginInput());
    } else {
        // Register tab
        m_registerButton->setEnabled(isValidRegisterInput());
    }
    
    m_statusLabel->hide();
}

void LoginDialog::onTabChanged(int index) {
    m_statusLabel->hide();
    validateInput();
}

bool LoginDialog::isValidLoginInput() const {
    return !m_loginUsernameEdit->text().trimmed().isEmpty() &&
           !m_loginPasswordEdit->text().isEmpty();
}

bool LoginDialog::isValidRegisterInput() const {
    QString username = m_registerUsernameEdit->text().trimmed();
    QString email = m_registerEmailEdit->text().trimmed();
    QString password = m_registerPasswordEdit->text();
    
    // Username validation (3-20 chars, alphanumeric + underscore)
    QRegularExpression usernameRegex("^[a-zA-Z0-9_]{3,20}$");
    if (!usernameRegex.match(username).hasMatch()) {
        return false;
    }
    
    // Email validation
    QRegularExpression emailRegex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    if (!emailRegex.match(email).hasMatch()) {
        return false;
    }
    
    // Password validation (minimum 6 characters)
    if (password.length() < 6) {
        return false;
    }
    
    return true;
}