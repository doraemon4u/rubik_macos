# rubik

一个使用 C++ 和 curses 库编写的 3D 魔方模拟器，在终端中运行，内含解魔方工具用于辅助求解。

## 构建

`build.sh` 自动检测操作系统并完成编译：

- **Windows**: 自动检测 MSYS2 (UCRT64 / MINGW64 / CLANG64) 环境，使用 PDCurses
- **Linux**:   自动检测 Linux 环境，使用 ncurses
- **macOS**:   自动检测 macOS，使用原生 Command Line Tools 的 clang，ncurses 随系统 SDK 提供
- 依赖缺失时会自动安装（Windows）或给出安装命令（Linux / macOS）

### macOS

1. 安装 [Xcode Command Line Tools](https://developer.apple.com/xcode/resources/)（含 clang / make / ncurses）：

```bash
xcode-select --install
```

2. 安装 cmake（Command Line Tools 不自带，需 [Homebrew](https://brew.sh/)）：

```bash
brew install cmake
```

3. 运行构建脚本：

```bash
cd /path/to/rubik
bash build.sh
```

脚本会优先使用 `/usr/bin/clang++`（原生 CLT 编译器）完成编译。

运行：

```bash
cd build_macos && ./rubik
```

> **终端建议**：键盘操作在任何终端下均可使用；鼠标拖拽旋转视角与滚轮缩放依赖
> 终端的鼠标事件上报（xterm mouse tracking），在 **iTerm2** 下体验最佳，
> 原生 Terminal.app 部分支持。macOS 系统自带 ncurses 版本较旧
> （NCURSES_MOUSE_VERSION 1），滚轮向下缩放可能不可用，可用 `-` 键替代。

### Windows

1. 安装 [MSYS2](https://www.msys2.org/)
2. 打开 **MSYS2 UCRT64** 终端（推荐）
3. 运行构建脚本：

```bash
cd /path/to/rubik
bash build.sh
```

脚本将自动通过 `pacman` 安装所需依赖（gcc/cmake/pdcurses/make），编译项目，并将必要的 DLL 复制到 `build_win/`。

运行：

```bash
cd build_win && ./rubik.exe
```

### Linux

```bash
cd /path/to/rubik
bash build.sh
```

如缺少依赖（g++/cmake/ncurses 开发库等），脚本会检测包管理器并给出对应的安装命令。

常见发行版手动安装依赖：

| 发行版        | 安装命令                                                       |
| ------------- | -------------------------------------------------------------- |
| Ubuntu/Debian | `sudo apt-get install -y build-essential cmake libncurses-dev` |
| Fedora        | `sudo dnf install -y gcc-c++ cmake make ncurses-devel`         |
| Arch          | `sudo pacman -S gcc cmake make ncurses`                        |
| CentOS/RHEL   | `sudo yum install -y gcc-c++ cmake make ncurses-devel`         |

运行：

```bash
cd build_linux && ./rubik
```

## 一键自动求解（rubik_autosolve）

构建脚本会同时编译出 `rubik_autosolve`：随机打乱魔方后自动求解，逐步还原到完美面，
每步操作之间有延时（默认 1 秒），实时渲染动画，可随时按 `q` / `ESC` 退出。

```bash
cd build_macos && ./rubik_autosolve            # 默认打乱 20 步，每步延时 1000ms
./rubik_autosolve 30                           # 打乱 30 步
./rubik_autosolve 20 500                       # 打乱 20 步，每步延时 500ms
```

## 操作说明

| 按键        | 功能              |
| ----------- | ----------------- |
| 方向键/鼠标 | 旋转魔方视角      |
| `+` / `-`   | 放大 / 缩小       |
| `C`         | 重置魔方          |
| `X`         | 随机打乱          |
| `Backspace` | 撤销上一步        |
| `f` / `F`   | 前面 顺/逆时针    |
| `b` / `B`   | 后面 顺/逆时针    |
| `l` / `L`   | 左面 顺/逆时针    |
| `r` / `R`   | 右面 顺/逆时针    |
| `u` / `U`   | 上面 顺/逆时针    |
| `d` / `D`   | 下面 顺/逆时针    |
| `ESC` / `q` | 退出              |
| `H`         | 开启/关闭提示系统 |
| `Space`     | 生成提示列表      |
| `=`         | 自动求解当前状态（每步 1 秒，`q` 中断） |