cmake_minimum_required(VERSION 2.8)
project(doorlockd)

option(USE_COLORIZED_LOGS "Colorized logging" ON)

set(DOORLOCK_VERSION_MAJOR 1)
set(DOORLOCK_VERSION_MINOR 0)
set(DOORLOCK_VERSION_PATCH 0)

set(DOORLOCK_VERSION "${DOORLOCK_VERSION_MAJOR}.${DOORLOCK_VERSION_MINOR}-${DOORLOCK_VERSION_PATCH}")

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

set(DOORLOCKD_SRCS
    src/clientmessage.cpp
    src/clientmessage.h
    src/daemon.cpp
    src/daemon.h
    src/door.cpp
    src/door.h
    src/doormessage.h
    src/logger.cpp
    src/logger.h
    src/logic.cpp
    src/logic.h
    src/response.cpp
    src/response.h
    src/util.cpp
    src/util.h

    src/doorlockd.cpp
)

set(DOORLOCK_CLIENT_SRCS
    src/clientmessage.cpp
    src/clientmessage.h
    src/doorlock-client.cpp
    src/logger.cpp
    src/logger.h
    src/response.cpp
    src/response.h
    src/util.cpp
    src/util.h
)

add_executable(doorlockd ${DOORLOCKD_SRCS})
target_link_libraries(doorlockd jsoncpp ldap ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(doorlock-client ${DOORLOCK_CLIENT_SRCS})
target_link_libraries(doorlock-client jsoncpp ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

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
