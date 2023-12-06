cmake_minimum_required(VERSION 3.15)

project(reversi VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS -g)

include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_SOURCE_DIR}/lib)

set(ENGINE_SRC src/board.cpp src/gameman.cpp src/engi.cpp src/reversi_widgets.cpp
    src/main_window.cpp src/mtcse.cpp)

file(COPY_FILE ${CMAKE_SOURCE_DIR}/static/board.bmp ${CMAKE_BINARY_DIR}/board.bmp)

add_executable(test_main test_main.cpp ${ENGINE_SRC})
add_executable(main src/main.cpp ${ENGINE_SRC})
target_compile_definitions(main PUBLIC DOCTEST_CONFIG_DISABLE)
target_link_libraries(main nana)
target_link_libraries(test_main nana)