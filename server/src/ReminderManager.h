#ifndef REMINDER_MANAGER_H
#define REMINDER_MANAGER_H

#include <thread>
#include <functional>
#include <atomic>
#include "Database.h"
#include "Event.h"

class ReminderManager {
public:
    explicit ReminderManager(Database* database);
    ~ReminderManager();
    
    void start();
    void stop();
    
    void setReminderCallback(std::function<void(const Event&)> callback);
    
private:
    void reminderLoop();
    void checkAndSendReminders();
    
    Database* m_database;
    std::thread m_thread;
    std::atomic<bool> m_running;
    std::function<void(const Event&)> m_reminderCallback;
};

#endif // REMINDER_MANAGER_H