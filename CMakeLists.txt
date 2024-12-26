cmake_minimum_required(VERSION 3.10)
project(airsense)

set(CMAKE_CXX_STANDARD 20)

add_executable(arisense_iot main.cpp)
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)
add_executable(sensor sensor.cpp)

target_link_libraries(sensor wiringPi CURL::libcurl nlohmann_json::nlohmann_json)
