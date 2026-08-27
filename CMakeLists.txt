cmake_minimum_required(VERSION 3.20)

project(DungeonsAndCats VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(dungeons_and_cats
    src/main.cpp
    src/core/Game.cpp
    src/input/InputManager.cpp
)

target_include_directories(dungeons_and_cats PRIVATE src)
target_link_libraries(dungeons_and_cats PRIVATE raylib)

if(MSVC)
    target_compile_options(dungeons_and_cats PRIVATE /W4 /permissive-)
else()
    target_compile_options(dungeons_and_cats PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(MINGW)
    target_link_options(dungeons_and_cats PRIVATE -static -static-libgcc -static-libstdc++)
endif()
