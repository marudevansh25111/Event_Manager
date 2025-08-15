#include "EventServer.h"
#include <iostream>
#include <signal.h>
#include <thread>

EventServer* g_server = nullptr;

void signal_handler(int signal) {
    std::cout << "\nShutting down server..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    int port = 8080;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }
    
    try {
        EventServer server;
        g_server = &server;
        
        server.start(port);
        
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        
        // Keep the main thread alive
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}