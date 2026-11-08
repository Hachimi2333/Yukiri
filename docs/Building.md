# 构建与集成

源码构建需要 CMake >= 3.28、Ninja、支持 C++23 的 MSVC 或 MinGW，以及 Vulkan SDK 1.4（本机使用 1.4.363.1）。Windows 运行需要支持 Vulkan 1.4 的驱动；SDK 不替代显卡驱动。

## 构建

在仓库根目录运行：

```powershell
git submodule update --init --recursive
./tools/Build.ps1 -Preset msvc-debug -Jobs 4 -Test -Install
./tools/Build.ps1 -Preset mingw-debug -Jobs 4 -Test -Install
```

将 debug 替换为 release 使用对应配置。产物分别位于 `out/build/<preset>`，安装目录为 `out/install/<preset>`。脚本用 vswhere 自动发现并加载 MSVC 开发者环境；MinGW 使用 PATH 中的 gcc / g++。`-Target` 指定目标，`-Fresh` 清除当前配置的 CMake 缓存并干净构建。

也可在已加载工具链的环境使用 `cmake --preset <preset>`、`cmake --build --preset <preset>` 和 `ctest --preset <preset>`。不同编译器 / 配置的二进制依赖不混用。

MSVC presets 设置 VSLANG；另窄范围修复中文 showIncludes 的 CMake 编码误探测。MinGW 的 Demo / 测试旁复制所选编译器的运行库 DLL，避免 PATH 冲突。Yukiri 和第三方组件均为静态库，系统 DLL、Vulkan 驱动、MSVC / MinGW 编译器运行库仍是运行要求。

## Demo 与检查

```powershell
./out/build/msvc-debug/YukiriDemo.exe
./tools/GpuSmoke.ps1 -Preset msvc-debug
./tools/VerifyPackage.ps1 -Preset msvc-debug
./tools/VerifyPackage.ps1 -Preset msvc-debug -Source
```

Demo 默认原生标题栏，可切换主题、打开其他窗口与自绘窗口。`--custom` 创建自绘主窗口，`--smoke` 运行自动 GPU 检查，`--idle-smoke` 验证空闲调度。设置 `YUKIRI_TRACE_FRAMES=1` 可在退出时打印窗口提交帧数。GPU 脚本检查超时、退出码与验证层日志，不加入默认 CTest。

## 源码集成

初始化 Yukiri 子模块后，在消费项目中使用：

```cmake
set(YUKIRI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YUKIRI_BUILD_DEMO OFF CACHE BOOL "" FORCE)
add_subdirectory(path/to/Yukiri)
target_link_libraries(MyApp PRIVATE Yukiri::Yukiri)
```

## 安装包集成

安装目录包含公开头文件、Yukiri 与所需静态依赖、CMake 元数据、许可证、文档和 Demo。整个目录可以移动；消费项目不需要 Vulkan SDK 或第三方头文件：

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyApp LANGUAGES CXX)
find_package(Yukiri 0.1 CONFIG REQUIRED)
add_executable(MyApp Main.cpp)
target_link_libraries(MyApp PRIVATE Yukiri::Yukiri)
```

配置时将 `CMAKE_PREFIX_PATH` 指向安装根目录。使用与包匹配的编译器家族、x64 架构、Debug / Release 和兼容运行库；包会拒绝已知不匹配。首版 presets / 包面向 Ninja 单配置生成器，每个配置使用独立安装前缀。MinGW 应用还需将对应编译器运行库加入 PATH 或放到程序旁。

`VerifyPackage.ps1` 复制安装目录后验证独立消费；`-Source` 验证 add_subdirectory。实际执行记录见 [Validation](Validation.md)。

## CI

GitHub Actions 配置 MSVC / MinGW × Debug / Release，构建、CPU 测试、安装并验证移动后的包。SDK 安装脚本仅用于一次性 CI runner。工作流不依赖 GPU，不将桌面输入法 / DPI 检查计为 CI 覆盖。尚未执行远端工作流。
