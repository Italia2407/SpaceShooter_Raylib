cmake_minimum_required(VERSION 3.0)
project(space_shooter C CXX)

set(CMAKE_C_STANDARD 99)

# Setting parameters for raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # or games

add_subdirectory(libs/raylib)

add_library(tracy libs/tracy/TracyClient.cpp)
target_include_directories(tracy PUBLIC libs/tracy)
target_compile_definitions(tracy PUBLIC TRACY_ENABLE)

add_executable(space_shooter src/main.c)
target_link_libraries(space_shooter PRIVATE raylib tracy)
target_compile_definitions(space_shooter PRIVATE ASSET_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/")