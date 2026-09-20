cmake_minimum_required(VERSION 3.15)
project(IntegracionCpp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# json.hpp (nlohmann/json) y exprtk.hpp son headers unicos: solo hace falta
# que existan en esta carpeta (cpp/), no requieren vcpkg ni find_package.
add_executable(principal principal.cpp)
target_include_directories(principal PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
