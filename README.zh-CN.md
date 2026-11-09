<img src="assets/Yukiri.svg" alt="Yukiri 图标" width="96" height="96">

# Yukiri

使用 Vulkan 渲染、外观现代简约的 C++23 桌面 GUI 框架。

[English](README.md) | **简体中文**

Yukiri 是以静态库交付的保留模式 GUI 框架，提供 C++ 控件树、类型化主题和 GPU 渲染，并配有交互式控件展示程序。当前版本为 **0.1.0**，先支持 Windows，Linux 支持处于规划中。

## 功能

- Vulkan 1.4 渲染，支持裁剪、圆角图形、透明叠放和文字图集。
- 保留模式控件，提供子控件独占所有权、布局约束、键盘焦点导航及信号连接。
- `StackPanel`、`Grid` 和 `Overlay` 布局。
- `Label`、`Button`、`CheckBox`、`RadioButton`、`ToggleSwitch`、`Slider`、`ProgressBar`、`TextBox`、`ComboBox`、`TabView` 和 `ScrollView`。
- UTF-8 文字、系统字体回退、FreeType / HarfBuzz 塑形，以及支持输入法预编辑、选区、剪贴板和撤销 / 重做的单行编辑。
- 跟随系统、浅色及深色主题，可配置的状态动画和平滑滚动。
- 多窗口，默认自绘标题栏，可显式选择原生装饰。
- FIFO 垂直同步、适应显示器刷新率的动画调度和按需重绘。
- 通过 `Yukiri::Yukiri` 提供 CMake 源码集成及可重定位安装包。

## 平台状态

| 平台 | 状态 |
|---|---|
| Windows 11 x64 | 已在本机验证 MSVC、MinGW 的 Debug 和 Release |
| Windows 10 22H2 x64 | 目标平台，尚待运行验证 |
| Linux | 规划中 |

Yukiri 仍处于早期阶段，公开 API 可能调整。完整双向段落、多行编辑、系统无障碍接口和更多控件列入后续规划。已实现范围与实际验收边界见[项目状态](docs/Status.md)、[验证记录](docs/Validation.md)和[路线图](FUTURE.md)。

0.2 开发分支已增加 UI 调度、定时器、命令、可取消关闭、位图 / SVG、字体选择、自适应布局、菜单、工具栏、提示、主题化模态对话框、异步原生文件对话框、多行编辑、查找替换、密码模式、数值输入、共享模型的虚拟化列表 / 树 / 表格，Windows UI Automation，以及文档 / 数据应用示例。新增能力目前具备编译器和 CPU 覆盖，GPU 与桌面验收分别记录在[里程碑清单](docs/DesktopFoundations.md)。

## 环境要求

在 Windows 上从源码构建需要：

- 支持 C++23 的 x64 MSVC 或 MinGW 工具链。
- CMake **3.28 或更新版本**、Ninja、Git 和 PowerShell。
- 包含 `glslc` 的 Vulkan SDK **1.4**；本机验证使用 SDK 1.4.363.1。
- MSVC 对应的 Windows SDK 资源工具，或 MinGW 的 `windres`，用于构建 Demo 图标。

运行 GUI 应用需要支持 **Vulkan 1.4** 的 GPU 和驱动。SDK 不能替代显卡驱动。

第三方库通过 Git submodule 锁定提交，从源码构建并静态链接。详见[依赖及许可证](docs/Dependencies.md)。常规构建不需要 SVG 转换工具或 Node.js。

## 快速开始

在 PowerShell 中运行：

```powershell
git clone --recurse-submodules https://github.com/Hachimi2333/Yukiri.git
cd Yukiri
./tools/Build.ps1 -Preset msvc-debug -Jobs 4 -Test
./out/build/msvc-debug/YukiriDemo.exe
```

已有仓库先初始化依赖：

```powershell
git submodule update --init --recursive
```

`Build.ps1` 通过 Visual Studio Installer 发现 MSVC 并加载开发者环境。使用 MinGW 时，先将匹配的 `gcc`、`g++` 和 `windres` 放入 `PATH`，然后运行：

```powershell
./tools/Build.ps1 -Preset mingw-debug -Jobs 4 -Test
./out/build/mingw-debug/YukiriDemo.exe
```

可用 presets 为 `msvc-debug`、`msvc-release`、`mingw-debug` 和 `mingw-release`，构建目录按配置隔离在 `out/build/<preset>`。添加 `-Install` 可安装到 `out/install/<preset>`。

已配置编译器环境时，也可以直接使用 CMake：

```powershell
cmake --preset msvc-debug
cmake --build --preset msvc-debug --parallel 4
ctest --preset msvc-debug
```

## Demo

`YukiriDemo` 展示各类控件、文字编辑、嵌套滚动、自定义 Canvas 绘制、主题切换和多窗口。新窗口继承创建来源窗口的主题设置。

Demo 默认使用自绘标题栏。可以点击原生标题栏按钮，或使用以下参数启动：

```powershell
./out/build/msvc-debug/YukiriDemo.exe --native
```

`--custom` 显式选择自绘装饰。Yukiri 图标嵌入 Demo 的 EXE；链接框架的应用不会自动携带 Yukiri 品牌。

## 集成

### 从源码构建

初始化 Yukiri 子模块后，将它加入 CMake 项目：

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyApp LANGUAGES CXX)

set(YUKIRI_BUILD_DEMO OFF CACHE BOOL "" FORCE)
set(YUKIRI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/Yukiri)

add_executable(MyApp Main.cpp)
target_link_libraries(MyApp PRIVATE Yukiri::Yukiri)
```

### 使用安装包

构建并安装所需配置：

```powershell
./tools/Build.ps1 -Preset msvc-release -Jobs 4 -Test -Install
```

在消费项目中使用：

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyApp LANGUAGES CXX)

find_package(Yukiri 0.1 CONFIG REQUIRED)
add_executable(MyApp Main.cpp)
target_link_libraries(MyApp PRIVATE Yukiri::Yukiri)
```

将 `CMAKE_PREFIX_PATH` 指向安装根目录。编译器家族、x64 架构、Debug / Release 配置及编译器运行库必须匹配。安装包面向 Ninja 单配置构建。使用安装包不需要 Vulkan SDK 或第三方头文件，但仍需要系统库、Vulkan 驱动及对应编译器运行库。

### 最小应用

```cpp
#include <Yukiri/Yukiri.hpp>
#include <iostream>
#include <memory>
#include <utility>

int main()
{
	auto application = Yukiri::Application::Create();
	if (!application)
	{
		std::cerr << application.error().m_Message << '\n';
		return 1;
	}
	auto window = (*application)->CreateWindow();
	if (!window)
	{
		std::cerr << window.error().m_Message << '\n';
		return 1;
	}
	auto root = std::make_unique<Yukiri::StackPanel>();
	root->SetPadding(Yukiri::Insets(24));
	root->AddChild<Yukiri::Label>("Hello, Yukiri!");
	(*window)->SetContent(std::move(root));
	auto result = (*application)->Run();
	if (!result) std::cerr << result.error().m_Message << '\n';
	return result ? 0 : 1;
}
```

公开头文件不暴露 SDL、Vulkan、FreeType 或 HarfBuzz 类型。所有权、事件、自定义控件和动画钩子见 [API 文档](docs/API.md)。

## 文档与开发

开发文档目前主要使用中文：

- [文档入口](docs/README.md)
- [构建、安装与集成](docs/Building.md)
- [架构](docs/Architecture.md)、[渲染](docs/Rendering.md)、[文字](docs/Text.md)和[样式](docs/Styling.md)
- [设计决策](docs/Decisions.md)、[项目状态](docs/Status.md)和[路线图](FUTURE.md)
- [桌面基础里程碑](docs/DesktopFoundations.md)与[用户启动的桌面检查](docs/ManualTesting.md)

参与开发前请阅读 [AGENTS.md](AGENTS.md)。保持改动职责集中，使用真实 Tab 缩进及既定 C++ 命名风格，同步相关文档；提交遵循 Conventional Commits，标题与说明正文均使用英文。

通过 `-Test` 运行 CPU 测试；GPU 和安装消费使用独立的本机工具：

```powershell
./tools/GpuSmoke.ps1 -Preset msvc-debug
./tools/VerifyPackage.ps1 -Preset msvc-debug -BuildOnly
./tools/VerifyPackage.ps1 -Preset msvc-debug -Source -BuildOnly
```

运行安装包消费检查前，先安装相同 preset。将实际编译器、GPU 及桌面操作结果记录在[验证记录](docs/Validation.md)。仓库按项目要求不配置 GitHub Actions 工作流。

## 许可证

Yukiri 及项目图标采用 [MIT 许可证](LICENSE)。第三方组件保留各自的许可证，安装包附带相关声明。
