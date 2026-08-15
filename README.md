# rubik

一个使用 C++ 和 curses 库编写的 3D 魔方模拟器，在终端中运行，内含解魔方工具用于辅助求解。

## 构建

`build.sh` 自动检测操作系统并完成编译：

- **Windows**: 自动检测 MSYS2 (UCRT64 / MINGW64 / CLANG64) 环境，使用 PDCurses
- **Linux**:   自动检测 Linux 环境，使用 ncurses
- 依赖缺失时会自动安装（Windows）或给出安装命令（Linux）

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