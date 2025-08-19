# Event Manager System

A real-time event management system built entirely in C++ with Qt6 GUI and WebSocket communication.
Client UI
<img width="590" height="468" alt="image" src="https://github.com/user-attachments/assets/cbd2dfcd-ae36-40eb-9b7f-175d0f31554a" />


## 🎯 Features

- **Real-time synchronization** across multiple clients
- **Smart reminder system** with configurable timing  
- **Cross-platform Qt6 client** (Windows, macOS, Linux)
- **Persistent SQLite storage** with automatic database creation
- **System tray integration** for background operation
- **Modern dark theme** UI with intuitive design
- **Cloud deployment ready** with Docker support
- **Automatic reminders** sent to all connected clients
- **Event CRUD operations** with instant updates

## 🏗️ Architecture

```
┌─────────────────┐    WebSocket    ┌─────────────────┐
│   Qt6 Clients   │ ←──────────────→ │  C++ Server     │
│  (Cross-platform)│                │  (Event Store)  │
└─────────────────┘                 └─────────────────┘
        │                                    │
        │                                    │
   ┌─────────┐                        ┌─────────────┐
   │ Qt GUI  │                        │  SQLite3    │
   │Widgets  │                        │ Database    │
   └─────────┘                        └─────────────┘
```

### Tech Stack
- **Backend**: C++ with WebSocket++, SQLite3, nlohmann/json
- **Frontend**: Qt6 (C++) with modern dark theme
- **Communication**: JSON over WebSockets
- **Database**: SQLite3 for persistence
- **Build System**: CMake
- **Deployment**: Docker + Docker Compose

## 🚀 Quick Start (macOS M3 Pro)

### 1. Setup Dependencies
```bash
# Clone or create project directory
mkdir event-manager && cd event-manager

# Copy all the files (from the artifacts above)
# Then run:

chmod +x scripts/*.sh
./scripts/install_deps_mac.sh
```

### 2. Build the System
```bash
./scripts/build_all.sh
```

### 3. Run Locally
```bash
# Terminal 1: Start server
./start_server.sh

# Terminal 2: Start client  
./start_client.sh
```

### 4. Use the Application
1. **Connect**: In client, click "Connect" (default: ws://localhost:8080)
2. **Add Events**: Click "Add Event" to create new events
3. **Real-time Sync**: Open multiple clients to see instant synchronization
4. **Reminders**: Create events with future times to test notifications

## 📁 Project Structure

```
event-manager/
├── server/                    # C++ WebSocket Server
│   ├── src/
│   │   ├── main.cpp
│   │   ├── EventServer.h/.cpp
│   │   ├── Database.h/.cpp
│   │   └── ReminderManager.h/.cpp
│   └── CMakeLists.txt
├── client/                    # Qt6 GUI Application  
│   ├── src/
│   │   ├── main.cpp
│   │   ├── MainWindow.h/.cpp/.ui
│   │   ├── EventDialog.h/.cpp/.ui
│   │   ├── WebSocketClient.h/.cpp
│   │   └── EventModel.h/.cpp
│   ├── resources/
│   └── CMakeLists.txt
├── shared/                    # Shared C++ Code
│   ├── Event.h/.cpp
│   └── Protocol.h/.cpp
├── scripts/                   # Build Scripts
│   ├── install_deps_mac.sh
│   ├── build_all.sh
│   └── deploy.sh
├── docker/                    # Docker Files
│   ├── Dockerfile.server
│   └── docker-compose.yml
└── README.md
```




**Built with ❤️ using C++17, Qt6, and modern development practices**
