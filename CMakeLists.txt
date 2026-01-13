cmake_minimum_required(VERSION 3.16)
project(spotify VERSION 0.2.1)

find_package(Albert REQUIRED)
find_package(Qt6Keychain REQUIRED)

albert_plugin(
    QT
        Concurrent
        Network
        Widgets
    LINK PRIVATE
        $<BUILD_LOCAL_INTERFACE:qt6keychain>
)

if(APPLE)
elseif(UNIX)
    albert_plugin_link_qt(DBus VERSION 6.0 REQUIRED)
endif()
