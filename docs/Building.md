# 构建与集成

0.2 开发默认使用不干扰桌面的验证：Build.ps1 的 -Test 只运行 CPU 测试，VerifyPackage.ps1 增加 -BuildOnly，只配置 / 编译 / 链接消费程序，不运行它。GPU 与桌面交互由用户按需启动；具体边界见 [实施清单](DesktopFoundations.md)。

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

Demo 默认自绘标题栏，主题下拉框旁的“深 / 浅”按钮一键切换当前实际主题；下拉框保留跟随系统。各窗口独立设置，新窗口继承创建来源的模式。`--native` 创建原生主窗口，`--custom` 保留为显式自绘选项，`--smoke` 运行自动 GPU 检查，`--idle-smoke` 验证空闲调度。设置 `YUKIRI_TRACE_FRAMES=1` 可在退出时打印窗口提交帧数。GPU 脚本检查超时、退出码与验证层日志，不加入默认 CTest。

Demo 的图标通过 RC 资源嵌入，MSVC 使用 Windows SDK 的 rc，MinGW 使用 windres。ICO 已提交到仓库，正常构建不需要 SVG 转换工具；运行 Demo 不需要旁置图标。原始 SVG 与尺寸说明见 assets，Yukiri 标识不链接到框架或消费项目。

## 源码集成

初始化 Yukiri 子模块后，在消费项目中使用：

```cmake
set(YUKIRI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YUKIRI_BUILD_DEMO OFF CACHE BOOL "" FORCE)
add_subdirectory(path/to/Yukiri)
target_link_libraries(MyApp PRIVATE Yukiri::Yukiri)
```

## 安装包集成

安装目录包含公开头文件、Yukiri 与所需静态依赖、CMake 元数据、许可证、文档和 Demo。安装文档位于 share/doc/Yukiri，保留根目录双语 README、开发规范、规划、许可证，以及 docs / assets 子目录，文档之间的相对链接与源码仓库一致。

整个目录可以移动；消费项目不需要 Vulkan SDK 或第三方头文件：

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyApp LANGUAGES CXX)
find_package(Yukiri 0.1 CONFIG REQUIRED)
add_executable(MyApp Main.cpp)
target_link_libraries(MyApp PRIVATE Yukiri::Yukiri)
```

配置时将 `CMAKE_PREFIX_PATH` 指向安装根目录。使用与包匹配的编译器家族、x64 架构、Debug / Release 和兼容运行库；包会拒绝已知不匹配。首版 presets / 包面向 Ninja 单配置生成器，每个配置使用独立安装前缀。MinGW 应用还需将对应编译器运行库加入 PATH 或放到程序旁。

`VerifyPackage.ps1` 复制安装目录后验证独立消费；`-Source` 验证 add_subdirectory。实际执行记录见 [Validation](Validation.md)。

## 本机验证

项目不提供 GitHub Actions 工作流。使用 Build.ps1、GpuSmoke.ps1 和 VerifyPackage.ps1 在本机分别完成构建、CPU 测试、GPU 检查和安装消费；桌面输入法 / DPI 检查单独记录。

## 性能检查

```powershell
$env:YUKIRI_FRAME_STATS = '1'
./out/build/msvc-release/YukiriFrameBenchmark.exe 8
./out/build/msvc-release/YukiriFrameBenchmark.exe 8 --scroll
./out/build/msvc-release/YukiriFrameBenchmark.exe 8 --scroll --settle
Remove-Item Env:YUKIRI_FRAME_STATS
```

第一项持续交替悬停，第二项在文本列表中连续反向滚动；均使用 SDL 内部事件，不移动系统鼠标。记录预热后的绘制 / 提交间隔、CPU、呈现调用和资源延后情况。scroll 模式另记录预热后的测量次数，用于检查滚动没有触发反复测量。--settle 在结束前停止输入，检查收稳后没有继续绘制；该模式至少运行 4 秒。

`--idle-smoke` 通过外部非阻塞 Poll 采样约 5.4 秒；常规 Run 在完全空闲时直接等待事件。`Poll(false)` 不保证每次都完成呈现，消费方需持续驱动循环。

MotionSettings 与焦点区域扩展改变了公开类型布局 / 虚函数表，现有应用应使用匹配工具链重新编译并重新链接。
