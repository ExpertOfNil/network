cmake_minimum_required(VERSION 3.16)
project(network LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/
)
include_directories(
	include
	/usr/include/
)

file(GLOB CLIENT_SOURCES ${CMAKE_SOURCE_DIR} "src/client/*.c")
file(GLOB SERVER_SOURCES ${CMAKE_SOURCE_DIR} "src/server/*.c")

add_executable(client
  ${CLIENT_SOURCES}
)

add_executable(server
  ${SERVER_SOURCES}
)

#target_link_libraries(
#  ${LIBS}
#  pthread
# )

