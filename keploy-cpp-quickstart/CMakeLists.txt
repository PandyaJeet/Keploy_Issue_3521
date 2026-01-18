cmake_minimum_required(VERSION 3.10)
project(keploy_cpp)

set(CMAKE_CXX_STANDARD 17)

add_executable(app src/main.cpp)

target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}/Crow/include
)

target_link_libraries(app pqxx pq)
