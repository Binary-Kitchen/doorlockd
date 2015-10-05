cmake_minimum_required(VERSION 2.8)
project(doorlockd)

option(USE_COLORIZED_LOGS "Colorized logging" ON)

set(DOORLOCK_VERSION_MAJOR 1)
set(DOORLOCK_VERSION_MINOR 1)
set(DOORLOCK_VERSION_PATCH 0)

set(DOORLOCK_VERSION "${DOORLOCK_VERSION_MAJOR}.${DOORLOCK_VERSION_MINOR}-${DOORLOCK_VERSION_PATCH}")

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

MESSAGE(STATUS "doorlockd version: ${DOORLOCK_VERSION}")

# Get the current working branch
execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(GIT_BRANCH "\"${GIT_BRANCH}\"")

# Get the latest abbreviated commit hash of the working branch
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(GIT_COMMIT_HASH "\"${GIT_COMMIT_HASH}\"")

add_definitions(-std=c++11)

configure_file (
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
)

include_directories(${PROJECT_BINARY_DIR})
include_directories(${Boost_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb -Wall -pedantic -Weffc++ -Wextra")
set(CMAKE_CXX_FLAGS "-O2 -Wall -pedantic -Wextra -Weffc++")
set(CMAKE_C_FLAGS_DEBUG "-O0 -ggdb -Wall -pedantic -Wextra")
set(CMAKE_C_FLAGS "-O2 -Wall -pedantic -Wextra")

find_package(Boost 1.55.0 COMPONENTS program_options system REQUIRED)

set(JSON_INCLUDE_DIR "/usr/include/jsoncpp" CACHE PATH "path to jsoncpp includes")
include_directories(${JSON_INCLUDE_DIR})

find_package (Threads)

find_package(Qt5Widgets)

set(LIBDOORLOCK_SRCS
    lib/clientmessage.cpp
    lib/clientmessage.h
    lib/door.cpp
    lib/door.h
    lib/doormessage.cpp
    lib/doormessage.h
    lib/logger.cpp
    lib/logger.h
    lib/logic.cpp
    lib/logic.h
    lib/request.cpp
    lib/request.h
    lib/response.cpp
    lib/response.h
    lib/util.cpp
    lib/util.h)

set(DOORLOCKD_SRCS
    daemon/doorlockd.cpp)

set(DOORLOCK_CLIENT_SRCS
    client/qrwidget.cpp
    client/qrwidget.h
    client/doorlock-client.cpp
    client/mainwindow.h
    client/mainwindow.cpp
    client/mainwindow.ui)

add_library(doorlock STATIC ${LIBDOORLOCK_SRCS})
target_link_libraries(doorlock jsoncpp ldap ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(doorlockd ${DOORLOCKD_SRCS})
target_link_libraries(doorlockd doorlock)

add_executable(doorlock-client ${DOORLOCK_CLIENT_SRCS})
target_link_libraries(doorlock-client doorlock qrencode Qt5::Widgets)
target_include_directories(doorlock-client PRIVATE ${CMAKE_SOURCE_DIR})

install(TARGETS doorlockd DESTINATION sbin/)
install(TARGETS doorlock-client DESTINATION bin/)

install(DIRECTORY images/ DESTINATION share/doorlockd/images
    FILES_MATCHING PATTERN "images/*.png"
    PERMISSIONS WORLD_READ OWNER_READ GROUP_READ)

install(FILES scripts/doorlockd.service DESTINATION /etc/systemd/system/)

install(DIRECTORY scripts/ DESTINATION etc/doorlockd/
    FILES_MATCHING PATTERN "scripts/pre_*lock"
    PERMISSIONS WORLD_EXECUTE WORLD_READ OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE OWNER_WRITE)

install(DIRECTORY scripts/ DESTINATION etc/doorlockd/
    FILES_MATCHING PATTERN "scripts/post_*lock"
    PERMISSIONS WORLD_EXECUTE WORLD_READ OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE OWNER_WRITE)

install(DIRECTORY scripts/ DESTINATION etc/doorlockd/
    FILES_MATCHING PATTERN "scripts/emergency_unlock"
    PERMISSIONS WORLD_EXECUTE WORLD_READ OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE OWNER_WRITE)
