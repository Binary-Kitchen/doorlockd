cmake_minimum_required(VERSION 2.8)
project(doorlockd)

option(USE_COLORIZED_LOGS "Colorized logging" ON)

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

set(SRCS
main.cpp
logger.cpp
door.cpp
logic.cpp
response.cpp
util.cpp
daemon.cpp
)

add_executable(doorlockd ${SRCS})
target_link_libraries(doorlockd jsoncpp ldap ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS doorlockd DESTINATION sbin/)
install(FILES img/template.png DESTINATION share/doorlockd/)
install(DIRECTORY scripts/ DESTINATION share/doorlockd/
    FILES_MATCHING PATTERN "scripts/doorlockd-*"
    PERMISSIONS WORLD_EXECUTE WORLD_READ OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE OWNER_WRITE)
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
