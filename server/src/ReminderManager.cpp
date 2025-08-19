#include "ReminderManager.h"
#include <iostream>
#include <thread>
#include <chrono>

ReminderManager::ReminderManager(Database* database) 
    : m_database(database), m_running(false) {
}

ReminderManager::~ReminderManager() {
    stop();
}

void ReminderManager::start() {
    if (m_running) return;
    
    m_running = true;
    m_thread = std::thread([this]() {
        reminderLoop();
    });
    
    std::cout << "Reminder manager started" << std::endl;
}

void ReminderManager::stop() {
    if (m_running) {
        m_running = false;
        if (m_thread.joinable()) {
            m_thread.join();
        }
        std::cout << "Reminder manager stopped" << std::endl;
    }
}

void ReminderManager::reminderLoop() {
    while (m_running) {
        checkAndSendReminders();
        std::this_thread::sleep_for(std::chrono::minutes(1)); // Check every minute
    }
}

// void ReminderManager::checkAndSendReminders() {
//     auto events = m_database->get_events_needing_reminder();
    
//     for (auto& event : events) {
//         if (event.needs_reminder()) {
//             // Send reminder through callback
//             if (m_reminderCallback) {
//                 m_reminderCallback(event);
//             }
            
//             // Mark reminder as sent
//             event.reminder_sent = true;
//             m_database->update_event(event);
            
//             std::cout << "Reminder sent for event: " << event.title << std::endl;
//         }
//     }
// }

void ReminderManager::checkAndSendReminders() {
    // Get ALL events that need reminders (not user-specific)
    auto events = m_database->get_events_needing_reminder();
    
    for (auto& event : events) {
        if (event.needs_reminder()) {
            // Send reminder to ALL users through callback
            if (m_reminderCallback) {
                m_reminderCallback(event);
            }
            
            // Mark reminder as sent
            event.reminder_sent = true;
            m_database->update_event(event);
            
            std::cout << "Reminder sent to all users for event: " << event.title 
                      << " (Created by User ID: " << event.user_id << ")" << std::endl;
        }
    }
}

void ReminderManager::setReminderCallback(std::function<void(const Event&)> callback) {
    m_reminderCallback = callback;
}