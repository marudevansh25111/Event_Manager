#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <memory>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QTabWidget;
class QLabel;
class QVBoxLayout;
class QFormLayout;
class QHBoxLayout;
QT_END_NAMESPACE

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    
    QString getUsername() const;
    QString getPassword() const;
    QString getEmail() const;
    QString getDisplayName() const;
    bool isRegistering() const;

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void validateInput();
    void onTabChanged(int index);

private:
    void setupUI();
    void setupLoginTab();
    void setupRegisterTab();
    bool isValidLoginInput() const;
    bool isValidRegisterInput() const;
    
    // UI components
    QTabWidget* m_tabWidget;
    
    // Login tab
    QWidget* m_loginTab;
    QLineEdit* m_loginUsernameEdit;
    QLineEdit* m_loginPasswordEdit;
    QPushButton* m_loginButton;
    
    // Register tab
    QWidget* m_registerTab;
    QLineEdit* m_registerUsernameEdit;
    QLineEdit* m_registerEmailEdit;
    QLineEdit* m_registerPasswordEdit;
    QLineEdit* m_registerDisplayNameEdit;
    QPushButton* m_registerButton;
    
    QLabel* m_statusLabel;
    bool m_isRegistering;
};

#endif // LOGINDIALOG_H