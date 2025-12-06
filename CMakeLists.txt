cmake_minimum_required(VERSION 3.16)
project(particle-simulator)

set(CMAKE_CXX_STANDARD 20)

find_package(SFML 2.5 COMPONENTS system window graphics REQUIRED)

add_executable(particle-simulator
    main.cpp
)

target_link_libraries(particle-simulator
    sfml-system
    sfml-window
    sfml-graphics
)

