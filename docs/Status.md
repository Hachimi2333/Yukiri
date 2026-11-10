# 项目状态

版本：**0.2.0**。本版本补齐 Windows 通用桌面应用的基础能力；下表区分实现与验收，不能把编译 / CPU 通过理解为完整桌面运行通过。

## 实现范围

| 领域 | 已实现内容 |
|---|---|
| 构建与交付 | C++23 静态库、CMake / Ninja、MSVC / MinGW Debug / Release、公开头自包含、源码集成及可重定位包 |
| 应用与窗口 | 多窗口、显示 / 隐藏 / 激活、所属关系、尺寸 / 状态及文件拖入通知、可取消关闭 |
| 任务与输入 | Dispatcher / Timer、完整按键与抬键、点击计数、捕获取消、共享 Command、快捷键与模态范围 |
| 渲染 | Vulkan 1.4、Dynamic Rendering / Synchronization2、FIFO / 刷新率调度、裁剪、圆角、透明度、文字图集及按需重绘 |
| 资源与文字 | 位图 / 静态 SVG / RGBA、帧 fence 生命周期、可增长可回收描述符、字体选择与回退、FreeType / HarfBuzz、缓存 |
| 布局与主题 | StackPanel / Grid / Overlay / Border / WrapPanel / SplitPanel、约束换行、浅深主题、基础动效和平滑滚动 |
| 基础控件 | Label、Button、CheckBox、RadioButton、ToggleSwitch、Slider、ProgressBar、TextBox、ComboBox、TabView、ScrollView、ImageView、NumberBox |
| 编辑 | TextArea、单行 / 多行字素编辑、自动换行、选区、预编辑、剪贴板、合并撤销 / 重做、查找替换、密码模式 |
| 应用交互 | 层级弹出内容、MenuBar / ContextMenu、ToolBar / StatusBar / Separator / ToolTip、Dialog / MessageDialog、可取消异步 FileDialog |
| 数据 | ItemModel / StandardItemModel / SelectionModel、虚拟 ListView / TreeView / TableView、稳定 ID、多选、排序、文本 / 勾选编辑 |
| 无障碍 | AccessibilityInfo、Windows UI Automation 模式、文字范围、按需虚拟节点与失效保护 |
| 窗口装饰 | 默认自绘 / 显式原生、系统按钮、拖动 / 缩放 / 系统菜单、HTMAXBUTTON、宿主 EXE 图标 |
| Demo | Gallery、Desktop 样例页、文档编辑闭环、共享数据管理、后台加载 / 取消、主题切换、多窗口 |

框架不嵌入 Yukiri 品牌资源。Demo 单独链接多尺寸图标，运行不需要旁置资源文件。九个顶层依赖（含仅测试 Catch2）固定 Git submodule 提交，库依赖全部从源码静态构建。

## 验证状态

- 本机 Windows 11 x64：MSVC / MinGW Debug / Release 的编译、CPU、公开头及包检查按 [Validation](Validation.md) 记录。
- 本轮自动检查不使用 Computer Use，不启动可见窗口、不注入桌面输入、不修改剪贴板。源码与安装消费只配置 / 编译 / 链接。
- 新能力的 GPU、长期资源使用、真实中文 IME、原生文件选择、Narrator、DPI / 跨屏、窗口装饰和完整 Demo 流程由用户启动，当前均待验收。步骤见 [ManualTesting](ManualTesting.md)，示例边界见 [Demo](Demo.md)。
- 历史 0.1 的四套配置各 46 项 CPU、GPU smoke 和消费运行记录保留在 Validation，不作为 0.2 新能力的运行验收。
- Windows 10 22H2 尚未实机运行。GitHub Actions 已按要求移除，使用本机工具维护验证。

## 已知边界

目标是中英文通用桌面应用，公开 API 仍可能调整，0.1 应用需要按 0.2 头文件重新编译 / 链接。包面向 Ninja 单配置，编译器、架构和 Debug / Release 不混用。应用 UI 线程使用 Windows COM STA。

多行为纯文本；查找为字面匹配；数据视图采用统一行高、行选择及基础文本 / 勾选编辑。SVG 支持自包含静态文档，不承诺浏览器级行为。密码限制针对剪贴板及无障碍读取，应用自身仍可访问值。

Linux、完整双向文字、彩色 Emoji、富文本、IDE、公式表格、停靠、浏览器、打印和完整跨应用拖放属于 [后续规划](../FUTURE.md)。
