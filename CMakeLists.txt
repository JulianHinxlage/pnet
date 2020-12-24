cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SOURCE_PLATFORM src/platform/linux/*.cpp)
file(GLOB_RECURSE SOURCE_SOCKET src/pnet/*.cpp)

project(pnet)
add_library(${PROJECT_NAME} STATIC ${SOURCE_SOCKET} ${SOURCE_PLATFORM})
target_include_directories(${PROJECT_NAME} PUBLIC src)
target_link_libraries(${PROJECT_NAME} PUBLIC pthread)

project(tcpTest)
add_executable(${PROJECT_NAME} src/test/tcpTest.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC pnet)

project(udpTest)
add_executable(${PROJECT_NAME} src/test/udpTest.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC pnet)

project(serverTest)
add_executable(${PROJECT_NAME} src/test/serverTest.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC pnet)

project(peerTest)
add_executable(${PROJECT_NAME} src/test/peerTest.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC pnet)

project(pnet)
