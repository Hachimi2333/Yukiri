# 构建与集成

需要 CMake >= 3.28、Ninja、支持 C++23 的 MSVC 或 MinGW，以及 Vulkan SDK 1.4。首轮验证环境包含 SDK 1.4.363.1。

在仓库根目录运行：

```powershell
git submodule update --init --recursive
./tools/Build.ps1 -Preset msvc-debug -Test
./tools/Build.ps1 -Preset mingw-debug -Test
```

将 debug 替换为 release 可构建对应 Release 配置。脚本自动发现并加载 MSVC 开发者环境；MinGW 使用 PATH 中的 gcc / g++。可通过 -Jobs 控制并行数，-Target 指定目标。

也可以在已配置的编译器环境中使用 cmake --preset、cmake --build --preset 和 ctest --preset。MSVC 与 MinGW 必须分别构建全部依赖，不能混用对象文件或静态库。

MSVC presets 固定 VSLANG；另对 CMake 的中文 showIncludes 编码误探测作窄范围修复。若复用旧构建目录，可用 -Fresh 重新配置并干净构建。MinGW 可执行文件旁会放置对应编译器的运行库 DLL，以免被 PATH 中其他工具的同名 DLL 覆盖。
