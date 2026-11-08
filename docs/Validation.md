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

- 文字：两套 Debug 的 13 项测试全部通过，包含中文、组合附加符、ZWJ 字素删除、编辑历史、非法 UTF-8、字体回退与 IME 预编辑模型。两套 GPU smoke 绘制中英文并保持验证层错误为零。
- MSVC 增量依赖：修复 CMake 将 UTF-8 中文 showIncludes 前缀经 CP936 错解码的问题，干净重建后 Ninja 已记录 Grid 等源文件的头依赖；不再使用零依赖的旧对象结果。

- 主题：两套 Debug 的 15 项测试全部通过，新增主题继承 / 覆盖和动画完成后停止调度检查。系统主题通知与原生标题栏外观接入 SDL / DWM，实际视觉检查随 Demo 执行。

- 控件：两套 Debug 的 21 项测试全部通过；新增按钮按下 / 释放、单选组、下拉键盘选择、滚动焦点可见性、标签页可见性与滑块范围检查。全部公共头独立编译通过。

## 尚未执行

双工具链完整构建、自动化测试、GPU 验证、中文 IME、DPI、跨显示器、两种标题栏与安装包消费验证。

Windows 10 22H2 尚无独立运行结果。编译通过不代表运行验证通过。
