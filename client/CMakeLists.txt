cmake_minimum_required(VERSION 3.16)
project(EventManagerClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build for x86_64 to match Intel Homebrew (same as server)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES x86_64)
endif()

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network WebSockets)

# Auto MOC, UIC, RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find nlohmann/json
find_package(nlohmann_json REQUIRED)

# OpenSSL - use Intel Homebrew installation (same as server)
if(APPLE)
    execute_process(
        COMMAND /usr/local/bin/brew --prefix openssl@3
        OUTPUT_VARIABLE BREW_OPENSSL_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    if(BREW_OPENSSL_PREFIX)
        set(OPENSSL_ROOT_DIR ${BREW_OPENSSL_PREFIX})
        set(OPENSSL_INCLUDE_DIR ${BREW_OPENSSL_PREFIX}/include)
        set(OPENSSL_CRYPTO_LIBRARY ${BREW_OPENSSL_PREFIX}/lib/libcrypto.dylib)
        set(OPENSSL_SSL_LIBRARY ${BREW_OPENSSL_PREFIX}/lib/libssl.dylib)
        
        if(EXISTS ${OPENSSL_CRYPTO_LIBRARY} AND EXISTS ${OPENSSL_SSL_LIBRARY})
            message(STATUS "Found OpenSSL at: ${BREW_OPENSSL_PREFIX}")
            set(OPENSSL_FOUND TRUE)
            set(OPENSSL_LIBRARIES ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
        endif()
    endif()
endif()

if(NOT OPENSSL_FOUND)
    find_package(OpenSSL REQUIRED)
    set(OPENSSL_LIBRARIES OpenSSL::SSL OpenSSL::Crypto)
endif()

# Include shared directory
include_directories(../shared)
include_directories(${OPENSSL_INCLUDE_DIR})

# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/EventDialog.cpp
    src/LoginDialog.cpp
    src/WebSocketClient.cpp
    src/EventModel.cpp
    ../shared/Event.cpp
    ../shared/Protocol.cpp
    ../shared/User.cpp
)

# Header files
set(HEADERS
    src/MainWindow.h
    src/EventDialog.h
    src/LoginDialog.h
    src/WebSocketClient.h
    src/EventModel.h
    ../shared/Event.h
    ../shared/Protocol.h
    ../shared/User.h
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
    ${OPENSSL_LIBRARIES}
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

# Debug info
message(STATUS "Building client for architecture: x86_64")
message(STATUS "OpenSSL Libraries: ${OPENSSL_LIBRARIES}")