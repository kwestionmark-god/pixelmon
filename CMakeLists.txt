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

# Copy generated JSON data next to the executable so it can be run from build/.
add_custom_command(TARGET pixelmon POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_SOURCE_DIR}/src/data/monsters.json $<TARGET_FILE_DIR:pixelmon>/monsters.json
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_SOURCE_DIR}/src/data/moves.json $<TARGET_FILE_DIR:pixelmon>/moves.json
)
