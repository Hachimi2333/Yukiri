# 项目状态

版本：0.1.0。以下为实现状态，具体测试环境和人工验收范围见 [验证记录](Validation.md)。

0.2 通用桌面基础里程碑正在独立分支开发，范围和实际进度见 [实施清单](DesktopFoundations.md)。不将规划中的新增能力计入当前完整支持范围。

0.2 已加入层级弹出内容、MenuBar / ContextMenu、ToolBar / StatusBar / Separator / ToolTip，以及所属窗口内 Dialog / MessageDialog 和独立 STA 的可取消 FileDialog。弹出内容保留退出视觉时立即退出输入，模态限制焦点及快捷键范围。真实系统对话框、菜单视觉和桌面交互尚待用户验收。

0.2 编辑阶段加入 TextArea 多行与段落缓存、字面查找替换、合并撤销、TextBox 密码模式、NumberBox，以及单选组、ComboBox、Slider 与不确定进度的标准键盘行为。CPU 几何 / 模型覆盖和真实 IME / 剪贴板验收分别记录。

0.2 数据阶段加入 ItemModel / StandardItemModel / SelectionModel、统一行高的虚拟化 ListView / TreeView / TableView、稳定 ID 选择、排序、列宽和基础文本 / 勾选编辑。十万行 CPU 检查覆盖热滚动访问量与控件数量，GPU 和实际体验仍待验收。

| 阶段 | 交付状态 |
|---|---|
| 开发规范与文档 | AGENTS.md、FUTURE.md 与 docs 已建立并持续维护 |
| 子模块与构建 | 九个依赖固定提交；MSVC / MinGW 的 Debug / Release Ninja presets |
| 平台与多窗口 | SDL 生命周期、独立窗口、事件循环、错误报告、关闭通知 |
| Vulkan 渲染 | 1.4 设备、Dynamic Rendering、Synchronization2、VMA、图集、批次、裁剪与交换链重建 |
| 控件树与布局 | 所有权、延迟销毁、路由、焦点、捕获、失效、StackPanel / Grid / Overlay |
| 文字与编辑 | 系统字体回退、FreeType / HarfBuzz、UTF-8 字素编辑、剪贴板、历史、IME |
| 主题 | 系统跟随、浅深主题、局部覆盖、焦点框和悬停过渡 |
| 基础控件 | 计划中的 12 种控件均已实现，含单行 TextBox 与窗口内下拉层 |
| 自绘标题栏 | 独立 Win32 适配、系统按钮、拖动 / 缩放 / 系统菜单、HTMAXBUTTON |
| Demo | 全控件交互展示、禁用状态、主题、多窗口、两种装饰及 Canvas 页面 |
| 安装与消费 | 静态依赖、许可证、可重定位 CMake 包；源码 / 已安装消费验证 |

0.1 交付基线的四套配置各通过 46 项 CPU 测试、GPU smoke 和移动安装目录后的独立消费；0.2 新功能按 Validation 的分阶段记录区分 CPU / 编译与待执行的 GPU 验收。验证通过本机工具执行；GitHub Actions 工作流及其专用安装脚本已移除。

首版仍有明确边界：Windows 10 未实机运行；实际 100% / 150% / 200% DPI、跨屏移动、最小化 / 拖动贴靠及 Snap Layouts 悬停菜单的完整桌面验收待补齐。返回 HTMAXBUTTON 和窗口样式已自动验证，不能替代 shell 菜单实际显示检查。

Linux、完整双向段落、彩色 Emoji、无障碍接口及扩展控件属于 [后续规划](../FUTURE.md)。

动画与流畅度优化已实现：MotionSettings、自定义动画钩子、按显示器刷新率调度、非阻塞资源检查、塑形缓存、帧内脏图集上传、控件状态动效、下拉收放、Tab 指示条和平滑滚动。Demo 提供与主题下拉框同步的深浅切换按钮；各窗口主题独立。

焦点显示修正：已区分鼠标与键盘模式，新增 IsFocusVisible / GetFocusBounds，修正选择类控件与 Tab 标题区域；本轮验证结果持续记录在 Validation。

输入法修正：已启用 SDL 框架内组合文本，预编辑下划线、组合选区、光标与候选区域共用显示数据；真实 IME 修正后的人工体验待用户试用确认。

框架与 Demo 现在默认自绘标题栏，原生标题栏通过显式配置、Demo 对比按钮或 --native 参数使用。平台 smoke 按明确窗口标题定位目标，同时验证默认自绘 / 显式原生及对应内容区域。

项目 SVG 已原样保存，Demo 嵌入多尺寸 Windows 图标；原生窗口、自绘标题栏和 EXE 使用同一标识。框架只提供内部宿主图标绘制能力，不包含品牌资源，不要求运行时图片文件；图标验证结果见 Validation。

仓库入口使用默认英文 README.md 与中文 README.zh-CN.md，提供一致的功能、构建、集成及状态说明；安装文档保留相同目录关系，便于离线阅读。
