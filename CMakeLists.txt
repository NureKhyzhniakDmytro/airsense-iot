cmake_minimum_required(VERSION 3.10)
project(airsense)

set(CMAKE_CXX_STANDARD 20)

find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)

add_executable(sensor sensor.cpp)
add_executable(device device.cpp)

target_link_libraries(sensor wiringPi CURL::libcurl nlohmann_json::nlohmann_json)
target_link_libraries(device wiringPi CURL::libcurl nlohmann_json::nlohmann_json)
