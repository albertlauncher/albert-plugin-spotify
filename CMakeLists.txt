cmake_minimum_required(VERSION 3.16)
project(spotify VERSION 0.4.0)

find_package(Albert REQUIRED)
find_package(Qt6Keychain REQUIRED)

albert_plugin(
    QT
        Concurrent
        Network
        Widgets
    LINK PRIVATE
        QCoro6::Core
        QCoro6::Network
        Qt6Keychain::Qt6Keychain
)

if(APPLE)
elseif(UNIX)
    albert_plugin_link_qt(DBus VERSION 6.0 REQUIRED)
endif()
