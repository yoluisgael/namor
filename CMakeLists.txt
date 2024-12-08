cmake_minimum_required(VERSION 3.27)
project(namor)

set(CMAKE_CXX_STANDARD 17)

add_executable(namor main.cpp graphics.cpp graphics.h)

set(SFML_STATIC_LIBRARIES TRUE)
set(SFML_DIR "C:/SFML/lib/cmake/SFML")
find_package(SFML COMPONENTS system window graphics audio REQUIRED)
include_directories(c:/SFML/include)
target_link_libraries(namor sfml-system sfml-window sfml-graphics sfml-audio)