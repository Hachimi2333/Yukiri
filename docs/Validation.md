# 验证记录

日期：2026-11-08。只将实际执行的项目列为通过。

## 本机环境

- Windows 11 x64，10.0.26300；实际窗口缩放 125%。
- CMake 4.4.3，Ninja 1.13.2。
- MSVC 19.51.36260 / Build Tools 18，工具集 14.51.36231。
- MinGW GCC 16.2.0，各工具链使用独立静态依赖产物。
- Vulkan SDK 1.4.363.1；实际选择 NVIDIA RTX 5060 Laptop GPU（Vulkan 1.4.357）。
- AMD Radeon 610M 可枚举（Vulkan 1.4.315），本轮未单独执行其运行验证。

## 自动化结果

| 配置 | 构建 | CTest | 平台 / Gallery GPU smoke | 可重定位安装包消费 |
|---|---|---|---|---|
| MSVC Debug | 通过 | 24 / 24 | 通过 | 通过 |
| MSVC Release | 通过 | 24 / 24 | 通过 | 通过 |
| MinGW Debug | 通过 | 24 / 24 | 通过 | 通过 |
| MinGW Release | 通过 | 24 / 24 | 通过 | 通过 |

CPU 测试覆盖 Signal 重入与连接寿命、布局约束 / Grid span、事件路由、焦点、分发和绘制期间销毁、主题继承、控件键盘行为、滚动可见性、下拉层边界、UTF-8 / 字素 / 单词边界、编辑历史、非法输入和 IME 模型。文字测量在逻辑 scale 1 / 1.5 / 2 下检查；这不代表对应 OS DPI 已人工验收。公共头文件以独立翻译单元编译。

平台 smoke 覆盖错误路径、原生 / 自绘双窗口、渲染回调替换内容、关闭通知和 Poll 重入拒绝；Win32 检查 WS_OVERLAPPEDWINDOW / 非 WS_POPUP 和 HTMAXBUTTON。Gallery 执行 70 帧主题 / 控件 / 标签页变化、360 DIP 窄窗口及恢复大小。两者均捕获进程退出码，Vulkan 验证层没有错误或 VUID 诊断。

安装消费先复制安装树到新目录，清空 VULKAN_SDK 并禁止查找 Vulkan SDK，再独立 configure / link / run。消费项目链接整个应用工厂路径，但其运行检查不需要 GPU。MSVC Debug 和 MinGW Debug 另外通过 add_subdirectory 源码集成。将 MSVC 包交给 MinGW 消费时，配置按预期拒绝并给出匹配工具链提示。

MSVC Debug 的 `--idle-smoke` 在等待事件的约 5.5 秒内只提交一帧。无持续重绘。MinGW 可执行文件使用编译器对应运行库，避免 PATH 中其他工具的同名 DLL。中文 MSVC showIncludes 探测修复后确认 Ninja 保存了头文件依赖，已进行过干净重建。

## 桌面操作结果

通过实际窗口操作观察：

- 默认原生 Gallery 的布局、中英文标签、浅 / 深主题切换。
- TextBox 中英文输入、全选，以及 Microsoft Pinyin 的候选窗口、提交和 Escape 取消；候选窗口出现在光标附近。
- 打开独立自绘窗口，标题区域与内容分离；最大化、恢复、右键系统菜单和关闭。关闭自绘窗口后原生窗口继续运行。
- 原生窗口关闭和多窗口生存期。

## 待验证

- Windows 10 22H2 实机 / 虚拟机运行。
- 实际 OS 100% / 150% / 200% DPI、跨显示器 DPI 改变与 IME 候选窗位置。
- 两种装饰的完整最小化、拖动、边缘缩放、双击标题最大化、贴靠矩阵。
- Windows 11 Snap Layouts 悬停菜单实际出现与区域选择。代码和自动检查满足 HTMAXBUTTON / 窗口样式要求，本轮没有观察到 shell 悬停菜单，不能声明人工验收通过。
- GitHub Actions 远端运行；当前仅配置工作流并本机解析脚本 / 工作流，未推送。

未执行的项目保留为验收缺口，不从编译或 smoke 结果推断通过。

## 动画优化：基础设施

MSVC Debug 与 MinGW Debug 各通过 27 项测试，新增缓动反向衔接、不同步长完成一致性、禁用 / 隐藏停止调度和动画回调销毁安全检查。

## 动画优化：帧调度与渲染

MSVC Debug / Release、MinGW Debug 均通过 30 项 CPU 测试和双窗口 Gallery GPU smoke，验证层无错误。新增刷新率 59.94 / 60 / 120 / 144 / 165 / 240Hz 的模拟、迟帧跳过、独立窗口 deadline、缓存复用 / 限额与字体加载失效检查。

SDL 本机读取刷新率约 240.002Hz。相同 FrameBenchmark 源码分别链接优化前 cda83bc 安装包和当前 MSVC Release 静态库，均启用验证层，运行约 8 秒：

| 指标 | 优化前 | 优化后 |
|---|---|---|
| 绘制次数 | 865 | 1902 |
| 预热绘制间隔 p50 | 7.6791ms | 4.1281ms |
| 预热绘制间隔 p95 | 16.687ms | 4.7088ms |
| 整个进程 CPU 时间 | 2421.875ms | 2187.5ms |
| Vulkan 验证错误 | 0 | 0 |

优化后的帧日志：预热 CPU p50 0.3019ms、p95 0.4715ms，呈现调用 p95 0.1258ms。结果来自当前硬件和桌面环境的单轮对比，不等同物理显示测量或所有应用的性能承诺。
