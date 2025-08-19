cmake_minimum_required(VERSION 3.16)
project(EventManagerServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force x86_64 architecture to match Intel Homebrew
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES x86_64)
endif()

# Suppress Boost warning
cmake_policy(SET CMP0167 NEW)

# Find packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# OpenSSL - use Intel Homebrew installation
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

# SQLite3
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# nlohmann/json
find_package(nlohmann_json REQUIRED)

# Include directories
include_directories(${SQLITE3_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(/usr/local/include)  # Intel Homebrew include path
include_directories(../shared)

# Source files
set(SOURCES
    src/main.cpp
    src/EventServer.cpp
    src/Database.cpp
    src/ReminderManager.cpp
    src/AuthManager.cpp
    ../shared/Event.cpp
    ../shared/Protocol.cpp
    ../shared/User.cpp
)

# Add executable
add_executable(event_server ${SOURCES})

# Link libraries
target_link_libraries(event_server 
    ${SQLITE3_LIBRARIES}
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    Threads::Threads
    nlohmann_json::nlohmann_json
)

# Compiler flags for macOS
if(APPLE)
    target_link_libraries(event_server "-framework CoreFoundation")
endif()

# Debug info
message(STATUS "Building for architecture: x86_64")
message(STATUS "OpenSSL Root: ${OPENSSL_ROOT_DIR}")
message(STATUS "OpenSSL Libraries: ${OPENSSL_LIBRARIES}")