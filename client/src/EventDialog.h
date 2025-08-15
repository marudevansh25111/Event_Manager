#ifndef EVENTDIALOG_H
#define EVENTDIALOG_H

#include <QDialog>
#include "Event.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QTextEdit;
class QDateTimeEdit;
class QSpinBox;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QFormLayout;
class QLabel;
QT_END_NAMESPACE

class EventDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EventDialog(QWidget *parent = nullptr);
    explicit EventDialog(const Event& event, QWidget *parent = nullptr);

    Event getEvent() const;

private slots:
    void onAccept();
    void onCancel();
    void validateInput();

private:
    void setupUI();
    void populateFields(const Event& event);
    bool isValidInput() const;

    // UI components
    QLineEdit* m_titleEdit;
    QTextEdit* m_descriptionEdit;
    QDateTimeEdit* m_eventTimeEdit;
    QSpinBox* m_reminderMinutesEdit;
    QLineEdit* m_creatorEdit;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    // Data
    Event m_originalEvent;
    bool m_editMode;
};

#endif // EVENTDIALOG_H