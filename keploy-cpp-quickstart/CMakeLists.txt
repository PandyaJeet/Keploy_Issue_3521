cmake_minimum_required(VERSION 3.10)
project(keploy_cpp)

set(CMAKE_CXX_STANDARD 17)

# Find dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

add_executable(app src/main.cpp)

target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}/Crow/include
    ${PQXX_INCLUDE_DIRS}
)

target_link_libraries(app 
    ${PQXX_LIBRARIES}
    pq
    pthread
)
