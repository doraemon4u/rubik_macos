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

# ============================================================================
# min2phase 解魔方器（Kociemba 两阶段算法，vendored: min2phaseCXX-master）
# 说明：仅编译求解所需的最小源文件集（info/CubieCube/coords/Search/min2phase）；
#       http.cpp 中的 server/webSearch 功能与终端应用无关且依赖 POSIX 接口，
#       故用 src/min2phase_http_stub.cpp 提供空实现（对 Windows 同样友好）。
# 许可证：min2phaseCXX 为自由软件（GPLv3），融入后随本程序一并分发其许可声明。
# ============================================================================
add_library(min2phase STATIC
    min2phaseCXX-master/src/info.cpp
    min2phaseCXX-master/src/CubieCube.cpp
    min2phaseCXX-master/src/coords.cpp
    min2phaseCXX-master/src/Search.cpp
    min2phaseCXX-master/src/tools.cpp
    min2phaseCXX-master/src/tests.cpp
    min2phaseCXX-master/src/min2phase.cpp
    src/min2phase_http_stub.cpp
)

target_include_directories(min2phase PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/min2phaseCXX-master/include
    ${CMAKE_CURRENT_SOURCE_DIR}/min2phaseCXX-master/src
)

# vendored coords.cpp 含未定义行为（-O3 的激进循环优化会将其误编译为死循环，
# 表现为 min2phase::init() 卡死）。统一以 -O2 编译该库，兼顾性能与正确性。
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(min2phase PRIVATE -O2)
endif()

# ----------------------------------------------------------------------------
# 主程序
# ----------------------------------------------------------------------------
add_executable(rubik
    src/main.cpp
    src/Vector3.cpp
    src/Quaternion.cpp
    src/ColorConverter.cpp
    src/RubiksCubePiece.cpp
    src/RubiksCube.cpp
    src/HintSystem.cpp
)

target_include_directories(rubik PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CURSES_INCLUDE_DIR}
    ${ADDITIONAL_INCLUDE_DIRS}
)

target_link_libraries(rubik ${CURSES_LIBRARIES} min2phase)

# ----------------------------------------------------------------------------
# 自测程序：验证 facelet 导出与解算闭环（rubik_selftest）
# ----------------------------------------------------------------------------
add_executable(rubik_selftest
    tests/selftest.cpp
    src/Vector3.cpp
    src/Quaternion.cpp
    src/ColorConverter.cpp
    src/RubiksCubePiece.cpp
    src/RubiksCube.cpp
    src/HintSystem.cpp
)

target_include_directories(rubik_selftest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CURSES_INCLUDE_DIR}
    ${ADDITIONAL_INCLUDE_DIRS}
)

target_link_libraries(rubik_selftest ${CURSES_LIBRARIES} min2phase)

# Windows 下需要链接 gdi32（部分 PDCurses 版本依赖）及 winmm（高精度定时器）
if(WIN32)
    target_link_libraries(rubik gdi32 comdlg32 winmm)
endif()
