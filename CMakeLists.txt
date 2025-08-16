cmake_minimum_required(VERSION 3.20)
set(PROJ_NAME "LcdSim")
project(${PROJ_NAME})

set(CMAKE_CSTANDARD 11)
set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)

add_executable(${PROJ_NAME}
    src/main.cpp
    src/gfx/fontx.cpp
    src/lcd/lcd_4bit.cpp
    src/gfx/lunagfx.cpp
)

include_directories(${PROJ_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(${PROJ_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/src
  ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(${PROJ_NAME} PRIVATE SDL2::SDL2)