# 0.2 桌面基础功能里程碑

本文件跟踪已确认计划的实际交付，未勾选内容不表示已经支持。

## 实施进度

- [x] 应用：Dispatcher / Timer、命令、完整输入、可取消关闭、所属窗口及通知（CPU / 编译验证；平台行为待用户验收）。
- [x] 图片：SDL_image / LunaSVG、公共图片、透明度与 GPU 资源回收（CPU / 编译验证；GPU 待用户验收）。
- [ ] 字体：字体族、字重、斜体与对应回退 / 缓存。
- [ ] 布局：Border / WrapPanel / SplitPanel、约束换行与文字对齐。
- [ ] 应用交互：分层弹出内容、菜单、工具栏、提示、模态及原生文件对话框。
- [ ] 编辑：TextArea、查找替换、密码模式、NumberBox 及现有控件交互补齐。
- [ ] 数据：ItemModel / StandardItemModel / SelectionModel、虚拟化列表 / 树 / 表格。
- [ ] 无障碍：Windows UI Automation、文字范围与虚拟化数据语义。
- [ ] Demo：文件编辑闭环、共享数据管理、后台加载进度。
- [ ] 交付：四套配置、安装消费、双语说明及用户验收清单。

## 验证边界

本轮不使用 Computer Use，不启动可见窗口，不注入桌面输入，不修改用户剪贴板。
自动执行编译、CPU 测试、公开头检查及仅编译 / 链接的消费检查。
GPU、真实中文 IME、原生文件对话框、Narrator、DPI / 跨屏和窗口交互由用户启动验收。
历史 GPU 检查不作为本轮新功能的运行验收。

## 已确认公共命名

Command、KeyGesture、Dispatcher、Timer、Image、Font、AccessibilityInfo、ItemModel、ItemId、SelectionModel、StandardItemModel、CloseRequestEvent。
新增控件使用 TextArea、ImageView、NumberBox、MenuBar、ContextMenu、Dialog、MessageDialog、FileDialog、ToolTip、Border、WrapPanel、SplitPanel、ToolBar、StatusBar、Separator、ListView、TreeView、TableView。

## 范围

目标是 Windows 通用桌面工具，保持 C++23 / Vulkan 1.4 / 静态库与主线程 UI。
多行控件使用纯文本；数据控件采用统一行高、行选择与基础文本 / 复选框编辑。
Linux、富文本、IDE、完整双向文字、彩色 Emoji、电子表格公式、停靠、浏览器、打印和完整跨应用拖放仍为后续工作。
