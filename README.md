# Yukiri

现代、简约的 C++23 保留模式 GUI 静态库，版本 **0.1.0**。使用 SDL3 和 Vulkan 1.4，首版实现 Windows x64，支持 MSVC、MinGW、CMake 与 Ninja。

包含独立控件树、布局、浅深主题、中英文文字与单行输入法编辑、多窗口，以及原生 / 自绘标题栏。`YukiriDemo` 展示全部首版控件和自定义 Canvas 绘制。

```powershell
git submodule update --init --recursive
./tools/Build.ps1 -Preset msvc-debug -Jobs 4 -Test
./out/build/msvc-debug/YukiriDemo.exe
```

将 `msvc-debug` 替换为 `msvc-release`、`mingw-debug` 或 `mingw-release` 使用其他配置。源码构建需要 Vulkan SDK 1.4；已安装包的使用者只需匹配的 C++23 工具链。

- [文档入口](docs/README.md)
- [构建、安装与集成](docs/Building.md)
- [API 与最小应用](docs/API.md)
- [架构](docs/Architecture.md)
- [项目状态](docs/Status.md) 与 [实际验证记录](docs/Validation.md)
- [开发规范](AGENTS.md) 与 [后续规划](FUTURE.md)

Windows 11 已完成本机验证；Windows 10、更多 DPI / 跨屏场景和远端 CI 尚待验证。详见验证记录。

许可证：[MIT](LICENSE)。第三方组件遵循各自许可证，安装包附带相关声明。
