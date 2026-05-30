cmake_minimum_required(VERSION 3.16)
project(spotify VERSION 0.4.0)

find_package(Albert REQUIRED)
find_package(Qt6Keychain REQUIRED)
find_package(QCoro6 REQUIRED COMPONENTS Coro Core Network)

albert_plugin(
    QT
        Concurrent
        Network
        Widgets
    LINK PRIVATE
        QCoro6::Coro
        QCoro6::Core
        QCoro6::Network
        $<BUILD_LOCAL_INTERFACE:qt6keychain>
)

if(APPLE)
elseif(UNIX)
    albert_plugin_link_qt(DBus VERSION 6.0 REQUIRED)
endif()
