cmake_minimum_required(VERSION 3.16)
project(particle-simulator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 2.5 COMPONENTS system window graphics REQUIRED)

add_executable(particle-simulator
    main.cpp
    World.cpp
    World.hpp
    Particle.hpp
    Config.hpp
)

target_link_libraries(particle-simulator
    sfml-system
    sfml-window
    sfml-graphics
)

target_include_directories(particle-simulator
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

