cmake_minimum_required(VERSION 3.16)
project(pixelmon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra)
endif()

# SFML: system, window, graphics (+ audio for later).
find_package(SFML 2.6 REQUIRED system window graphics audio)

add_executable(pixelmon
  src/main.cpp
  src/types.cpp
  src/core/Game.cpp
  src/core/BattleSystem.cpp
  src/core/World.cpp
  src/core/Map.cpp
  src/core/MapStore.cpp
  src/ui/SpriteCache.cpp
  src/data/DataRepository.cpp
)

target_include_directories(pixelmon PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/external
)

target_link_libraries(pixelmon PRIVATE
  sfml-system sfml-window sfml-graphics sfml-audio
)

# Keep generated JSON data in sync next to the executable so it can be run
# from build/. Runs every build (regenerated maps need no recompile).
add_custom_target(copy-data ALL
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_SOURCE_DIR}/src/data/monsters.json ${CMAKE_BINARY_DIR}/monsters.json
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_SOURCE_DIR}/src/data/moves.json ${CMAKE_BINARY_DIR}/moves.json
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/src/data/maps ${CMAKE_BINARY_DIR}/maps
)
add_dependencies(pixelmon copy-data)
