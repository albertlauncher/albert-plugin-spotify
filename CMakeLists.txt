cmake_minimum_required(VERSION 3.16)
project(spotify VERSION 0.1)

find_package(Albert REQUIRED)

albert_plugin(QT Widgets Network)

if(APPLE)
elseif(UNIX)
    albert_plugin_link_qt(DBus VERSION 6.0 REQUIRED)
endif()
