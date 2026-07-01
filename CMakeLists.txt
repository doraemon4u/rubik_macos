cmake_minimum_required(VERSION 3.10)
project(rubik)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    # Windows: 查找 PDCurses（MSYS2 安装路径）
    # 优先从环境变量 PDCURSES_DIR 查找，也支持 MSYS2 默认路径
    # PDCurses 在 MSYS2 中 curses.h 位于 include/pdcurses/ 子目录
    find_path(CURSES_INCLUDE_DIR curses.h
        HINTS
            $ENV{PDCURSES_DIR}/include
            $ENV{MINGW_PREFIX}/include
        PATHS
            /mingw64/include
            /ucrt64/include
            /mingw32/include
        PATH_SUFFIXES pdcurses
    )
    # 同时查找 pdcurses.h 的父目录（用于 #include <pdcurses/curses.h> 风格）
    find_path(PDCURSES_ROOT_INCLUDE_DIR pdcurses.h
        HINTS
            $ENV{PDCURSES_DIR}/include
            $ENV{MINGW_PREFIX}/include
        PATHS
            /mingw64/include
            /ucrt64/include
            /mingw32/include
    )

    find_library(CURSES_LIBRARIES
        NAMES pdcurses pdcurses.a libpdcurses.a curses
        HINTS
            $ENV{PDCURSES_DIR}/lib
            $ENV{MINGW_PREFIX}/lib
        PATHS
            /mingw64/lib
            /ucrt64/lib
            /mingw32/lib
    )

    # 如果找到 pdcurses.h 所在目录，也加入包含路径
    if(PDCURSES_ROOT_INCLUDE_DIR)
        list(APPEND ADDITIONAL_INCLUDE_DIRS ${PDCURSES_ROOT_INCLUDE_DIR})
    endif()

    if(NOT CURSES_INCLUDE_DIR OR NOT CURSES_LIBRARIES)
        message(FATAL_ERROR
            "PDCurses not found!\n"
            "Please install it via MSYS2:\n"
            "  pacman -S mingw-w64-x86_64-pdcurses\n"
            "or set PDCURSES_DIR to the PDCurses installation directory."
        )
    endif()

    message(STATUS "Found PDCurses: ${CURSES_LIBRARIES}")
    message(STATUS "PDCurses include: ${CURSES_INCLUDE_DIR}")
else()
    # Linux/macOS: 使用系统 ncurses
    find_package(Curses REQUIRED)
    set(CURSES_INCLUDE_DIR ${CURSES_INCLUDE_DIRS})
endif()

add_executable(rubik
    src/main.cpp
    src/Vector3.cpp
    src/Quaternion.cpp
    src/ColorConverter.cpp
    src/RubiksCubePiece.cpp
    src/RubiksCube.cpp
)

target_include_directories(rubik PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CURSES_INCLUDE_DIR}
    ${ADDITIONAL_INCLUDE_DIRS}
)

target_link_libraries(rubik ${CURSES_LIBRARIES})

# Windows 下需要链接 gdi32（部分 PDCurses 版本依赖）及 winmm（高精度定时器）
if(WIN32)
    target_link_libraries(rubik gdi32 comdlg32 winmm)
endif()
