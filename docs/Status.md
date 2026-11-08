# 项目状态

版本：0.1.0。以下为实现状态，具体测试环境和人工验收范围见 [验证记录](Validation.md)。

| 阶段 | 交付状态 |
|---|---|
| 开发规范与文档 | AGENTS.md、FUTURE.md 与 docs 已建立并持续维护 |
| 子模块与构建 | 七个依赖固定提交；MSVC / MinGW 的 Debug / Release Ninja presets |
| 平台与多窗口 | SDL 生命周期、独立窗口、事件循环、错误报告、关闭通知 |
| Vulkan 渲染 | 1.4 设备、Dynamic Rendering、Synchronization2、VMA、图集、批次、裁剪与交换链重建 |
| 控件树与布局 | 所有权、延迟销毁、路由、焦点、捕获、失效、StackPanel / Grid / Overlay |
| 文字与编辑 | 系统字体回退、FreeType / HarfBuzz、UTF-8 字素编辑、剪贴板、历史、IME |
| 主题 | 系统跟随、浅深主题、局部覆盖、焦点框和悬停过渡 |
| 基础控件 | 计划中的 12 种控件均已实现，含单行 TextBox 与窗口内下拉层 |
| 自绘标题栏 | 独立 Win32 适配、系统按钮、拖动 / 缩放 / 系统菜单、HTMAXBUTTON |
| Demo | 全控件交互展示、禁用状态、主题、多窗口、两种装饰及 Canvas 页面 |
| 安装与消费 | 静态依赖、许可证、可重定位 CMake 包；源码 / 已安装消费验证 |

四套配置各通过 24 项 CPU 测试、GPU smoke 和移动安装目录后的独立消费。GitHub Actions 已配置，尚未在远端执行。

首版仍有明确边界：Windows 10 未实机运行；实际 100% / 150% / 200% DPI、跨屏移动、最小化 / 拖动贴靠及 Snap Layouts 悬停菜单的完整桌面验收待补齐。返回 HTMAXBUTTON 和窗口样式已自动验证，不能替代 shell 菜单实际显示检查。

Linux、完整双向段落、彩色 Emoji、无障碍接口及扩展控件属于 [后续规划](../FUTURE.md)。
