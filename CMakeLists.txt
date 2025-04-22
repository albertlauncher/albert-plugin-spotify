cmake_minimum_required(VERSION 3.16)
project(spotify VERSION 1.0)

find_package(Albert REQUIRED)

albert_plugin(
    INCLUDE PRIVATE
        $<TARGET_PROPERTY:albert::mediaremote,INTERFACE_INCLUDE_DIRECTORIES>
    LINK PRIVATE
        albert::mediaremote
    QT
        Widgets Network
)
