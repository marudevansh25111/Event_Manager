cmake_minimum_required(VERSION 3.16)
project(EventManagerServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# SQLite3
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# nlohmann/json
find_package(nlohmann_json REQUIRED)

# Include directories
include_directories(${SQLITE3_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(/opt/homebrew/include)
include_directories(../shared)

# Add executable
add_executable(event_server
    src/main.cpp
    src/EventServer.cpp
    src/Database.cpp
    src/ReminderManager.cpp
    ../shared/Event.cpp
    ../shared/Protocol.cpp
)

# Link libraries
target_link_libraries(event_server 
    ${SQLITE3_LIBRARIES}
    ${Boost_LIBRARIES}
    OpenSSL::SSL 
    OpenSSL::Crypto
    Threads::Threads
    nlohmann_json::nlohmann_json
)

# Compiler flags for macOS
if(APPLE)
    target_link_libraries(event_server "-framework CoreFoundation")
endif()