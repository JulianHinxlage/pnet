cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SOURCE_PLATFORM src/platform/linux/*.cpp)
file(GLOB_RECURSE SOURCE_SOCKET src/socket/*.cpp)

project(socket)
add_library(${PROJECT_NAME} STATIC ${SOURCE_SOCKET} ${SOURCE_PLATFORM})
target_include_directories(${PROJECT_NAME} PUBLIC src)

project(tcpTest)
add_executable(${PROJECT_NAME} src/test/tcpTest.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC socket)

project(udpTest)
add_executable(${PROJECT_NAME} src/test/udpTest.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC socket)

project(socket)
