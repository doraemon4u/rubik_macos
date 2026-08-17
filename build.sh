#!/usr/bin/env bash
# =====================================================
#  Rubik's Cube — 跨平台构建脚本
#  自动检测 Windows (MSYS2) / Linux / macOS，检查依赖，完成编译
# =====================================================
set -e

# ---------- 颜色输出 ----------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

info()  { echo -e "${CYAN}[INFO]${NC}  $*"; }
ok()    { echo -e "${GREEN}[OK]${NC}    $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
err()   { echo -e "${RED}[ERROR]${NC} $*"; }
header(){ echo; echo -e "${GREEN}============================================${NC}"; echo -e "${GREEN}  $*${NC}"; echo -e "${GREEN}============================================${NC}"; echo; }

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
VERSION="1.0.0"
DIST_DIR="$SCRIPT_DIR/dist"

# =====================================================
#  第 1 步：检测操作系统
# =====================================================
header "Rubik's Cube — Cross-Platform Build"

detect_os() {
    case "$(uname -s)" in
        MINGW*|MSYS*)
            # Windows (MSYS2 / Git Bash)
            OS="windows"
            if [ -d "/ucrt64" ]; then
                MINGW_ROOT="/ucrt64"
                PKG_PREFIX="mingw-w64-ucrt-x86_64"
                FLAVOR="UCRT64"
            elif [ -d "/mingw64" ]; then
                MINGW_ROOT="/mingw64"
                PKG_PREFIX="mingw-w64-x86_64"
                FLAVOR="MINGW64"
            elif [ -d "/clang64" ]; then
                MINGW_ROOT="/clang64"
                PKG_PREFIX="mingw-w64-clang-x86_64"
                FLAVOR="CLANG64"
            else
                err "检测到 MSYS2 环境，但未找到 /ucrt64 /mingw64 /clang64 目录。"
                err "请使用 MSYS2 UCRT64 / MINGW64 / CLANG64 终端运行此脚本。"
                exit 1
            fi
            BUILD_DIR="$SCRIPT_DIR/build_win"
            EXECUTABLE="$BUILD_DIR/rubik.exe"
            GENERATOR="MinGW Makefiles"
            CURSES_PKG="${PKG_PREFIX}-pdcurses"
            CMAKE_PKG="${PKG_PREFIX}-cmake"
            GCC_PKG="${PKG_PREFIX}-gcc"
            MAKE_PKG="${PKG_PREFIX}-make"
            info "检测到 Windows (MSYS2 $FLAVOR)"
            ;;
        Linux)
            OS="linux"
            BUILD_DIR="$SCRIPT_DIR/build_linux"
            EXECUTABLE="$BUILD_DIR/rubik"
            GENERATOR="Unix Makefiles"
            info "检测到 Linux"
            ;;
        Darwin)
            OS="macos"
            BUILD_DIR="$SCRIPT_DIR/build_macos"
            EXECUTABLE="$BUILD_DIR/rubik"
            GENERATOR="Unix Makefiles"
            info "检测到 macOS"
            ;;
        *)
            err "不支持的操作系统: $(uname -s)"
            err "当前仅支持 Windows (MSYS2)、Linux 和 macOS。"
            exit 1
            ;;
    esac
}

detect_os

# =====================================================
#  第 2 步：检查/安装依赖
# =====================================================
header "第 1 步：检查依赖"

check_windows_deps() {
    local missing=()

    # 检查编译器 (g++)
    if ! command -v g++ &>/dev/null; then
        missing+=("g++ (${GCC_PKG})")
    fi

    # 检查 cmake
    if ! command -v cmake &>/dev/null; then
        missing+=("cmake (${CMAKE_PKG})")
    fi

    # 检查 make / mingw32-make
    if ! command -v make &>/dev/null && ! command -v mingw32-make &>/dev/null; then
        missing+=("make (${MAKE_PKG})")
    fi

    # 检查 PDCurses (通过 pkg-config 或头文件)
    local pdcurses_found=false
    if pkg-config --exists pdcurses 2>/dev/null; then
        pdcurses_found=true
    elif [ -f "${MINGW_ROOT}/include/pdcurses.h" ] || [ -f "${MINGW_ROOT}/include/curses.h" ]; then
        pdcurses_found=true
    fi

    if [ "$pdcurses_found" = false ]; then
        missing+=("PDCurses (${CURSES_PKG})")
    fi

    if [ ${#missing[@]} -gt 0 ]; then
        warn "以下依赖未安装："
        for m in "${missing[@]}"; do
            warn "  - $m"
        done
        echo
        warn "正在尝试通过 pacman 自动安装..."

        if ! command -v pacman &>/dev/null; then
            err "未找到 pacman 包管理器！"
            err "请确保你在 MSYS2 终端中运行，然后手动执行："
            echo
            echo "  pacman -Syu"
            echo "  pacman -S ${GCC_PKG} ${CMAKE_PKG} ${MAKE_PKG} ${CURSES_PKG}"
            echo
            exit 1
        fi

        pacman -S --noconfirm --needed \
            "${GCC_PKG}" \
            "${CMAKE_PKG}" \
            "${MAKE_PKG}" \
            "${CURSES_PKG}" || {
                err "自动安装失败，请手动执行："
                echo
                echo "  pacman -S ${GCC_PKG} ${CMAKE_PKG} ${MAKE_PKG} ${CURSES_PKG}"
                echo
                exit 1
            }
    fi

    ok "Windows 依赖检查通过"
}

check_linux_deps() {
    local missing=()

    # 检查编译器 (g++)
    if ! command -v g++ &>/dev/null; then
        if command -v clang++ &>/dev/null; then
            ok "使用 clang++ 作为编译器"
            export CC=clang
            export CXX=clang++
        else
            missing+=("g++ (build-essential / gcc-c++)")
        fi
    fi

    # 检查 cmake
    if ! command -v cmake &>/dev/null; then
        missing+=("cmake")
    fi

    # 检查 make
    if ! command -v make &>/dev/null; then
        missing+=("make (build-essential)")
    fi

    # 检查 ncurses 开发库
    if ! pkg-config --exists ncurses 2>/dev/null; then
        # 尝试检测 ncurses 头文件
        if ! ldconfig -p 2>/dev/null | grep -q libncurses || \
           ! find /usr/include -name "curses.h" -maxdepth 3 2>/dev/null | grep -q .; then
            missing+=("ncurses 开发库 (libncurses-dev / ncurses-devel)")
        fi
    fi

    if [ ${#missing[@]} -gt 0 ]; then
        warn "以下依赖未安装："
        for m in "${missing[@]}"; do
            warn "  - $m"
        done
        echo

        # 尝试检测包管理器并给出安装命令
        local mgr=""
        local install_cmd=""

        if command -v apt-get &>/dev/null; then
            mgr="apt"
            install_cmd="sudo apt-get install -y build-essential cmake libncurses-dev"
        elif command -v dnf &>/dev/null; then
            mgr="dnf"
            install_cmd="sudo dnf install -y gcc-c++ cmake make ncurses-devel"
        elif command -v yum &>/dev/null; then
            mgr="yum"
            install_cmd="sudo yum install -y gcc-c++ cmake make ncurses-devel"
        elif command -v pacman &>/dev/null; then
            mgr="pacman"
            install_cmd="sudo pacman -S --needed gcc cmake make ncurses"
        elif command -v zypper &>/dev/null; then
            mgr="zypper"
            install_cmd="sudo zypper install -y gcc-c++ cmake make ncurses-devel"
        elif command -v apk &>/dev/null; then
            mgr="apk"
            install_cmd="sudo apk add g++ cmake make ncurses-dev"
        fi

        if [ -n "$mgr" ]; then
            err "检测到包管理器: ${mgr}"
            err "请运行以下命令安装依赖："
            echo
            echo "  ${install_cmd}"
            echo
        else
            err "未检测到已知的包管理器，请手动安装以下依赖："
            for m in "${missing[@]}"; do
                echo "  - $m"
            done
            echo
            err "常见发行版安装方式："
            echo "  Ubuntu/Debian:  sudo apt-get install -y build-essential cmake libncurses-dev"
            echo "  Fedora:         sudo dnf install -y gcc-c++ cmake make ncurses-devel"
            echo "  CentOS/RHEL:    sudo yum install -y gcc-c++ cmake make ncurses-devel"
            echo "  Arch:           sudo pacman -S gcc cmake make ncurses"
            echo "  openSUSE:       sudo zypper install -y gcc-c++ cmake make ncurses-devel"
            echo "  Alpine:         sudo apk add g++ cmake make ncurses-dev"
            echo
        fi
        exit 1
    fi

    ok "Linux 依赖检查通过"
}

check_macos_deps() {
    local missing=()

    # 编译器：优先使用 Xcode Command Line Tools 自带的原生 clang++
    if [ -x /usr/bin/clang++ ]; then
        export CC=/usr/bin/clang
        export CXX=/usr/bin/clang++
        ok "使用原生 Command Line Tools clang++"
    elif command -v clang++ &>/dev/null; then
        export CC=clang
        export CXX=clang++
        ok "使用 clang++ 作为编译器"
    else
        missing+=("clang++ (Xcode Command Line Tools)")
    fi

    # cmake：Command Line Tools 不自带，需通过 Homebrew 安装
    if ! command -v cmake &>/dev/null; then
        missing+=("cmake (brew install cmake)")
    fi

    # make：Command Line Tools 自带
    if ! command -v make &>/dev/null; then
        missing+=("make (Xcode Command Line Tools)")
    fi

    # ncurses：随 macOS SDK 提供，检查 SDK 头文件即可
    local sdk_path
    sdk_path="$(xcrun --show-sdk-path 2>/dev/null || true)"
    if [ -z "$sdk_path" ] || [ ! -f "$sdk_path/usr/include/curses.h" ]; then
        missing+=("ncurses 开发库（需安装 Xcode Command Line Tools）")
    fi

    if [ ${#missing[@]} -gt 0 ]; then
        err "以下依赖未安装："
        for m in "${missing[@]}"; do
            warn "  - $m"
        done
        echo
        err "macOS 安装指引："
        echo "  1. 安装 Xcode Command Line Tools:  xcode-select --install"
        echo "  2. 安装 cmake:                      brew install cmake"
        echo
        exit 1
    fi

    ok "macOS 依赖检查通过"
}

if [ "$OS" = "windows" ]; then
    check_windows_deps
elif [ "$OS" = "macos" ]; then
    check_macos_deps
else
    check_linux_deps
fi

# =====================================================
#  第 3 步：CMake 配置
# =====================================================
header "第 2 步：CMake 配置"

mkdir -p "$BUILD_DIR"

if [ "$OS" = "windows" ]; then
    cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
        -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE=Release
else
    cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
        -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE=Release
fi

ok "CMake 配置完成 → $BUILD_DIR"

# =====================================================
#  第 4 步：编译
# =====================================================
header "第 3 步：编译"

JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
info "使用 $JOBS 个并行任务"

cmake --build "$BUILD_DIR" --config Release -j"$JOBS"

ok "编译完成"

# =====================================================
#  第 5 步：后处理
# =====================================================
header "第 4 步：后处理"

if [ "$OS" = "windows" ]; then
    # Windows: 复制运行时 DLL
    info "复制运行时 DLL..."
    for dll in libgcc_s_seh-1.dll libgcc_s_dw2-1.dll libwinpthread-1.dll libstdc++-6.dll libpdcurses.dll; do
        src="${MINGW_ROOT}/bin/${dll}"
        if [ -f "$src" ]; then
            cp "$src" "$BUILD_DIR/"
            info "  ✓ $dll"
        fi
    done
fi

# =====================================================
#  第 5 步：打包
#   Windows: zip（exe + 运行时 DLL）   Linux: cpack → .deb   macOS: hdiutil → .dmg
# =====================================================
header "第 5 步：打包"

mkdir -p "$DIST_DIR"

if [ "$OS" = "windows" ]; then
    PKG_NAME="rubik-${VERSION}-win64.zip"
    PKG_PATH="$DIST_DIR/$PKG_NAME"
    if command -v zip &>/dev/null; then
        (cd "$BUILD_DIR" && zip -qr "$PKG_PATH" rubik.exe rubik_autosolve.exe rubik_selftest.exe *.dll)
    else
        # 兜底：PowerShell 压缩（MSYS2 未装 zip 时）
        WIN_BUILD="$(cygpath -w "$BUILD_DIR")"
        WIN_PKG="$(cygpath -w "$PKG_PATH")"
        powershell -NoProfile -Command "Compress-Archive -Path '${WIN_BUILD}\\*' -DestinationPath '${WIN_PKG}' -Force"
    fi
    if [ -f "$PKG_PATH" ]; then
        ok "打包完成 → $PKG_PATH"
    else
        warn "Windows 打包失败（需要 zip 或 PowerShell）"
    fi
elif [ "$OS" = "linux" ]; then
    if command -v dpkg-deb &>/dev/null; then
        # deb 约定安装到 /usr/bin，重配前缀后由 cpack 生成 .deb
        cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
            -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
        cpack -G DEB --config "$BUILD_DIR/CPackConfig.cmake" -B "$BUILD_DIR"
        if mv "$BUILD_DIR"/*.deb "$DIST_DIR/" 2>/dev/null; then
            ok "打包完成 → $(ls "$DIST_DIR"/*.deb)"
        else
            warn "cpack 未产出 .deb 文件"
        fi
    else
        warn "未找到 dpkg-deb，跳过 .deb 打包（Ubuntu/Debian 需安装 dpkg）"
    fi
else
    # macOS: 拖放式 dmg（CLI 工具通用做法）
    STAGE="$DIST_DIR/dmg-stage"
    rm -rf "$STAGE"
    mkdir -p "$STAGE"
    cp "$BUILD_DIR/rubik" "$BUILD_DIR/rubik_autosolve" "$SCRIPT_DIR/README.md" "$STAGE/"
    hdiutil create -volname Rubik -srcfolder "$STAGE" -ov -format UDZO \
        "$DIST_DIR/rubik-${VERSION}-macos.dmg"
    rm -rf "$STAGE"
    ok "打包完成 → $DIST_DIR/rubik-${VERSION}-macos.dmg"
fi

# =====================================================
#  完成
# =====================================================
header "编译成功！"

echo "  平台:       $([ "$OS" = "windows" ] && echo "Windows (MSYS2 $FLAVOR)" || [ "$OS" = "macos" ] && echo "macOS" || echo "Linux")"
echo "  构建目录:   $BUILD_DIR"
echo "  可执行文件: $EXECUTABLE"
echo
if [ "$OS" = "windows" ]; then
    echo "运行: cd build_win && ./rubik.exe"
else
    echo "运行: cd build_$OS && ./rubik"
fi
echo
