cmake_minimum_required(VERSION 2.8)
project(doorlockd)

option(USE_COLORIZED_LOGS "Colorized logging" ON)

add_definitions(-std=c++11)

configure_file (
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
)

include_directories("${PROJECT_BINARY_DIR}")

set(SRCS
main.cpp
logger.cpp
door.cpp
epaper.cpp
logic.cpp
util.cpp
epaper/Display_COG_Process.c
epaper/Display_Controller.c
epaper/Display_Hardware_Driver.c
epaper/bsp.c
)

add_executable(doorlockd ${SRCS})

target_link_libraries(doorlockd wiringPi jsoncpp)
