# 验证记录

## 环境

- OS：Windows 11 x64，版本 10.0.26300。
- CMake 4.4.3；Ninja 1.13.2。
- MinGW GCC 16.2.0；MSVC Build Tools 18，工具集 14.51。
- Vulkan SDK 1.4.363.1。
- NVIDIA RTX 5060 Laptop GPU：Vulkan 1.4.357；AMD Radeon 610M：Vulkan 1.4.315。

## 已执行

- 规划阶段：仓库与工具链检查；MinGW C++23 expected / unique_ptr 语法检查通过。
- 开发规范阶段：检查文档链接与 Git 差异。

- 构建骨架：MSVC Debug / MinGW Debug 从子模块源码完成静态依赖与 Yukiri 构建。

## 尚未执行

双工具链完整构建、自动化测试、GPU 验证、中文 IME、DPI、跨显示器、两种标题栏与安装包消费验证。

Windows 10 22H2 尚无独立运行结果。编译通过不代表运行验证通过。
