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

- 平台层：MSVC Debug / MinGW Debug 均通过双窗口 smoke；覆盖重复应用、非法尺寸、关闭单窗口和退出。此时尚未接入 GPU 绘制。

- Vulkan：MSVC Debug / MinGW Debug 在 RTX 5060 上均通过双窗口各八帧清屏与呈现，验证层错误计数为零。尚未以此声称复杂控件绘制已验证。

- UI 核心：两套 Debug 的 8 项 Catch2 / CTest 测试全部通过；覆盖信号重入、连接寿命、布局、Grid span、焦点和分发期间销毁。
- 绘制：两套 Debug 的 GPU smoke 已加入圆角、边框、透明叠放、矩形裁剪与线条，验证层错误为零。
- MinGW 运行库：复制所选编译器的 DLL 到可执行文件目录，避免 PATH 中其他工具的同名 DLL 干扰。

## 尚未执行

双工具链完整构建、自动化测试、GPU 验证、中文 IME、DPI、跨显示器、两种标题栏与安装包消费验证。

Windows 10 22H2 尚无独立运行结果。编译通过不代表运行验证通过。
