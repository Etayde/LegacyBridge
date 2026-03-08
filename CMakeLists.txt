cmake_minimum_required(VERSION 3.10)

project(LegacyBridge VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(httplib)

find_package(OpenSSL REQUIRED)

add_executable(LegacyBridge main.cpp utils.cpp POSWatchdog.cpp CloudUploader.cpp)
target_link_libraries(LegacyBridge PRIVATE nlohmann_json::nlohmann_json httplib::httplib OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(LegacyBridge PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
target_include_directories(LegacyBridge PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)