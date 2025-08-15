cmake_minimum_required(VERSION 3.16)
project(EventManagerClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network WebSockets)

# Auto MOC, UIC, RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find nlohmann/json
find_package(nlohmann_json REQUIRED)

# Include shared directory
include_directories(../shared)

# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/EventDialog.cpp
    src/WebSocketClient.cpp
    src/EventModel.cpp
    ../shared/Event.cpp
    ../shared/Protocol.cpp
)

# Header files
set(HEADERS
    src/MainWindow.h
    src/EventDialog.h
    src/WebSocketClient.h
    src/EventModel.h
    ../shared/Event.h
    ../shared/Protocol.h
)

# UI files
set(UI_FILES
    src/MainWindow.ui
    src/EventDialog.ui
)

# Resource files
set(RESOURCES
    resources/resources.qrc
)

# Create executable
add_executable(event_client
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${RESOURCES}
)

# Link Qt6 libraries
target_link_libraries(event_client
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::WebSockets
    nlohmann_json::nlohmann_json
)

# Platform-specific settings
if(APPLE)
    set_target_properties(event_client PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER com.eventmanager.client
        MACOSX_BUNDLE_BUNDLE_VERSION 1.0
        MACOSX_BUNDLE_SHORT_VERSION_STRING 1.0
    )
endif()