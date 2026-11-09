<img src="assets/Yukiri.svg" alt="Yukiri logo" width="96" height="96">

# Yukiri

A C++23 desktop GUI framework with Vulkan rendering and a simple, modern appearance.

**English** | [简体中文](README.zh-CN.md)

Yukiri is a retained-mode GUI framework distributed as a static library. It combines a C++ widget tree, typed themes, and GPU rendering, with an interactive control gallery. The current version is **0.1.0**, focused on Windows; Linux support is planned.

## Features

- Vulkan 1.4 rendering with clipping, rounded shapes, transparency, and text atlases.
- Retained widgets with exclusive child ownership, layout constraints, keyboard focus navigation, and signal connections.
- `StackPanel`, `Grid`, and `Overlay` layouts.
- `Label`, `Button`, `CheckBox`, `RadioButton`, `ToggleSwitch`, `Slider`, `ProgressBar`, `TextBox`, `ComboBox`, `TabView`, and `ScrollView`.
- UTF-8 text, system font fallback, FreeType / HarfBuzz shaping, and single-line editing with IME composition, selection, clipboard, and undo / redo.
- System, light, and dark themes; configurable state animations and smooth scrolling.
- Multiple windows, custom title bars by default, and optional native decorations.
- FIFO vertical synchronization, refresh-aware animation scheduling, and redraws only when needed.
- CMake source integration and relocatable packages through `Yukiri::Yukiri`.

## Platform status

| Platform | Status |
|---|---|
| Windows 11 x64 | Locally validated with MSVC and MinGW, Debug and Release |
| Windows 10 22H2 x64 | Targeted; runtime validation pending |
| Linux | Planned |

Yukiri is at an early stage; public APIs may change. Full bidirectional paragraphs, multiline editing, accessibility integration, and additional controls are planned. See [project status](docs/Status.md), [validation records](docs/Validation.md), and the [roadmap](FUTURE.md) for implemented and tested boundaries.

The 0.2 development branch adds UI dispatch, timers, commands, cancelable close requests, and public raster/SVG image resources. New capabilities currently have compiler and CPU coverage; GPU and desktop acceptance are tracked separately in the [milestone checklist](docs/DesktopFoundations.md).

## Requirements

To build from source on Windows:

- An x64 MSVC or MinGW toolchain with C++23 support.
- CMake **3.28 or newer**, Ninja, Git, and PowerShell.
- Vulkan SDK **1.4**, including `glslc`; local validation uses SDK 1.4.363.1.
- Windows SDK resource tools for MSVC, or `windres` for MinGW, to build the Demo icon.

Running a GUI application requires a GPU and driver supporting **Vulkan 1.4**. The SDK does not replace the graphics driver.

Third-party libraries are pinned Git submodules and built from source as static dependencies. See [dependencies and licenses](docs/Dependencies.md). Normal builds do not require an SVG converter or Node.js.

## Quick start

Run the following in PowerShell:

```powershell
git clone --recurse-submodules https://github.com/Hachimi2333/Yukiri.git
cd Yukiri
./tools/Build.ps1 -Preset msvc-debug -Jobs 4 -Test
./out/build/msvc-debug/YukiriDemo.exe
```

For an existing checkout, initialize the dependencies first:

```powershell
git submodule update --init --recursive
```

`Build.ps1` discovers MSVC through the Visual Studio Installer and loads its developer environment. For MinGW, put the matching `gcc`, `g++`, and `windres` on `PATH`, then run:

```powershell
./tools/Build.ps1 -Preset mingw-debug -Jobs 4 -Test
./out/build/mingw-debug/YukiriDemo.exe
```

Available presets are `msvc-debug`, `msvc-release`, `mingw-debug`, and `mingw-release`. Each has an isolated build directory under `out/build/<preset>`. Add `-Install` to install into `out/install/<preset>`.

In an environment where the compiler is already configured, the equivalent CMake commands are:

```powershell
cmake --preset msvc-debug
cmake --build --preset msvc-debug --parallel 4
ctest --preset msvc-debug
```

## Demo

`YukiriDemo` includes a control gallery, editable text, nested scrolling, custom Canvas drawing, theme switching, and additional windows. New windows inherit the creating window's theme setting.

The Demo starts with a custom title bar. Use its native title bar button or launch it with:

```powershell
./out/build/msvc-debug/YukiriDemo.exe --native
```

`--custom` explicitly selects custom decorations. The Yukiri icon is embedded in the Demo executable; applications linking the framework do not inherit Yukiri branding.

## Integration

### Build from source

Initialize Yukiri's submodules, then add it to your CMake project:

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyApp LANGUAGES CXX)

set(YUKIRI_BUILD_DEMO OFF CACHE BOOL "" FORCE)
set(YUKIRI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/Yukiri)

add_executable(MyApp Main.cpp)
target_link_libraries(MyApp PRIVATE Yukiri::Yukiri)
```

### Use an installed package

Build and install the desired configuration:

```powershell
./tools/Build.ps1 -Preset msvc-release -Jobs 4 -Test -Install
```

In the consuming project:

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyApp LANGUAGES CXX)

find_package(Yukiri 0.1 CONFIG REQUIRED)
add_executable(MyApp Main.cpp)
target_link_libraries(MyApp PRIVATE Yukiri::Yukiri)
```

Point `CMAKE_PREFIX_PATH` to the installation root. Match the compiler family, x64 architecture, Debug / Release configuration, and compatible compiler runtime. Packages target Ninja single-configuration builds. Installed-package consumers do not need the Vulkan SDK or third-party headers; system libraries, the Vulkan driver, and the appropriate compiler runtime are still required.

### Minimal application

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

Public headers do not expose SDL, Vulkan, FreeType, or HarfBuzz types. See the [API guide](docs/API.md) for ownership, events, custom widgets, and animation hooks.

## Documentation and development

Most developer documentation is currently in Chinese:

- [Documentation index](docs/README.md)
- [Building, packaging, and integration](docs/Building.md)
- [Architecture](docs/Architecture.md), [rendering](docs/Rendering.md), [text](docs/Text.md), and [styling](docs/Styling.md)
- [Design decisions](docs/Decisions.md), [project status](docs/Status.md), and [roadmap](FUTURE.md)
- [Desktop foundations milestone](docs/DesktopFoundations.md) and [user-run desktop checks](docs/ManualTesting.md)

Before contributing, read [AGENTS.md](AGENTS.md). Keep changes focused, use actual tabs and the established C++ naming conventions, update the relevant documentation, and use Conventional Commits with explanatory English bodies.

CPU tests run with `-Test`. GPU and package checks are separate local tools:

```powershell
./tools/GpuSmoke.ps1 -Preset msvc-debug
./tools/VerifyPackage.ps1 -Preset msvc-debug -BuildOnly
./tools/VerifyPackage.ps1 -Preset msvc-debug -Source -BuildOnly
```

Install the same preset before running the installed-package check. Record actual compiler, GPU, and desktop interaction results in [Validation.md](docs/Validation.md). The repository intentionally has no GitHub Actions workflow.

## License

Yukiri is licensed under the [MIT License](LICENSE), including the project icon. Third-party components retain their own licenses; the installed package includes the relevant notices.
