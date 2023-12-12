cmake_minimum_required(VERSION 3.15)

project(reversi VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall)

include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_SOURCE_DIR}/lib)

set(ENGINE_SRC src/board.cpp src/gameman.cpp src/engi.cpp src/reversi_widgets.cpp
    src/main_window.cpp src/mctse.cpp)
set(TEST_SRC src/test_board.cpp test_main.cpp)

file(COPY_FILE ${CMAKE_SOURCE_DIR}/static/board.bmp ${CMAKE_BINARY_DIR}/board.bmp)
file(COPY_FILE ${CMAKE_SOURCE_DIR}/static/appicon.ico ${CMAKE_BINARY_DIR}/appicon.ico)

add_library(reversi STATIC ${ENGINE_SRC})
add_executable(test_main ${TEST_SRC})
target_link_libraries(test_main reversi nana)
add_executable(main src/main.cpp src/main.rc)
target_link_libraries(main reversi nana)
