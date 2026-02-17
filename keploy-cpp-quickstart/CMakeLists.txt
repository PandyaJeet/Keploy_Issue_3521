cmake_minimum_required(VERSION 3.14)
project(keploy_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Fetch Crow (header-only web framework) ──────────────────────────────
include(FetchContent)

FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        v1.2.0
    GIT_SHALLOW    TRUE
)
# Crow is header-only; we only need the headers, skip building tests/examples
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(Crow)

# ── Find libpqxx (PostgreSQL C++ client) ────────────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# ── Build the application ───────────────────────────────────────────────
add_executable(app src/main.cpp)

target_include_directories(app PRIVATE
    ${PQXX_INCLUDE_DIRS}
)

target_link_libraries(app
    Crow::Crow
    ${PQXX_LIBRARIES}
    pq
    pthread
)
