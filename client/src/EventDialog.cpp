#include "EventDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QDateTime>

EventDialog::EventDialog(QWidget *parent)
    : QDialog(parent), m_editMode(false)
{
    setupUI();
    setWindowTitle("Add New Event");
    
    // Set default values
    m_eventTimeEdit->setDateTime(QDateTime::currentDateTime().addSecs(3600)); // 1 hour from now
    m_reminderMinutesEdit->setValue(60); // 60 minutes before
}

EventDialog::EventDialog(const Event& event, QWidget *parent)
    : QDialog(parent), m_originalEvent(event), m_editMode(true)
{
    setupUI();
    setWindowTitle("Edit Event");
    populateFields(event);
}

void EventDialog::setupUI()
{
    setModal(true);
    resize(400, 300);

    // Create layouts
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QFormLayout* formLayout = new QFormLayout();
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    // Create form fields
    m_titleEdit = new QLineEdit();
    m_titleEdit->setPlaceholderText("Enter event title...");
    
    m_descriptionEdit = new QTextEdit();
    m_descriptionEdit->setPlaceholderText("Enter event description...");
    m_descriptionEdit->setMaximumHeight(100);
    
    m_eventTimeEdit = new QDateTimeEdit();
    m_eventTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
    m_eventTimeEdit->setCalendarPopup(true);
    m_eventTimeEdit->setMinimumDateTime(QDateTime::currentDateTime());
    
    m_reminderMinutesEdit = new QSpinBox();
    m_reminderMinutesEdit->setRange(1, 10080); // 1 minute to 1 week
    m_reminderMinutesEdit->setSuffix(" minutes before");
    m_reminderMinutesEdit->setValue(60);
    
    m_creatorEdit = new QLineEdit();
    m_creatorEdit->setPlaceholderText("Your name...");

    // Add fields to form
    formLayout->addRow("Title*:", m_titleEdit);
    formLayout->addRow("Description:", m_descriptionEdit);
    formLayout->addRow("Event Time*:", m_eventTimeEdit);
    formLayout->addRow("Reminder:", m_reminderMinutesEdit);
    formLayout->addRow("Creator:", m_creatorEdit);

    // Create buttons
    m_okButton = new QPushButton("OK");
    m_cancelButton = new QPushButton("Cancel");
    
    m_okButton->setDefault(true);
    m_okButton->setEnabled(false); // Initially disabled

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);

    // Add layouts to main layout
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(m_titleEdit, &QLineEdit::textChanged, this, &EventDialog::validateInput);
    connect(m_eventTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &EventDialog::validateInput);
    connect(m_okButton, &QPushButton::clicked, this, &EventDialog::onAccept);
    connect(m_cancelButton, &QPushButton::clicked, this, &EventDialog::onCancel);

    // Initial validation
    validateInput();
}

void EventDialog::populateFields(const Event& event)
{
    m_titleEdit->setText(QString::fromStdString(event.title));
    m_descriptionEdit->setPlainText(QString::fromStdString(event.description));
    m_creatorEdit->setText(QString::fromStdString(event.creator));
    
    // Convert std::chrono::time_point to QDateTime
    auto time_t = std::chrono::system_clock::to_time_t(event.event_time);
    QDateTime eventDateTime = QDateTime::fromSecsSinceEpoch(time_t);
    m_eventTimeEdit->setDateTime(eventDateTime);
    
    // Calculate reminder minutes
    auto reminderDuration = std::chrono::duration_cast<std::chrono::minutes>(
        event.event_time - event.reminder_time);
    m_reminderMinutesEdit->setValue(static_cast<int>(reminderDuration.count()));
}

Event EventDialog::getEvent() const
{
    Event event;
    
    if (m_editMode) {
        event = m_originalEvent; // Keep original ID and other properties
    }
    
    event.title = m_titleEdit->text().trimmed().toStdString();
    event.description = m_descriptionEdit->toPlainText().trimmed().toStdString();
    event.creator = m_creatorEdit->text().trimmed().toStdString();
    
    // Convert QDateTime to std::chrono::time_point
    QDateTime eventDateTime = m_eventTimeEdit->dateTime();
    auto time_t = eventDateTime.toSecsSinceEpoch();
    event.event_time = std::chrono::system_clock::from_time_t(time_t);
    
    // Calculate reminder time
    int reminderMinutes = m_reminderMinutesEdit->value();
    event.reminder_time = event.event_time - std::chrono::minutes(reminderMinutes);
    
    return event;
}

void EventDialog::onAccept()
{
    if (isValidInput()) {
        accept();
    }
}

void EventDialog::onCancel()
{
    reject();
}

void EventDialog::validateInput()
{
    m_okButton->setEnabled(isValidInput());
}

bool EventDialog::isValidInput() const
{
    // Title is required
    if (m_titleEdit->text().trimmed().isEmpty()) {
        return false;
    }
    
    // Event time must be in the future (unless editing existing event)
    QDateTime eventTime = m_eventTimeEdit->dateTime();
    if (!m_editMode && eventTime <= QDateTime::currentDateTime()) {
        return false;
    }
    
    return true;
}