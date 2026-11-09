# Demo 应用流程

Gallery 保留原控件、主题、多窗口和 Canvas 页面，增加“Desktop / 桌面基础”页及两个工作场景入口。默认采用自绘标题栏；从 Gallery 打开时继承来源窗口的装饰和主题。以下命令由用户自行运行，本轮自动检查不启动窗口。

```powershell
./out/build/msvc-release/YukiriDemo.exe
./out/build/msvc-release/YukiriDemo.exe --editor
./out/build/msvc-release/YukiriDemo.exe --data --native
```

## 文档编辑

- 文件菜单 / 工具栏 / 快捷键共享 Command：Ctrl+N 新建、Ctrl+O 打开、Ctrl+S 保存、Ctrl+Shift+S 另存为、Ctrl+W 关闭。
- TextArea 支持自动换行、查找 / 替换 / 全部替换、撤销 / 重做、剪贴板、右键菜单、中文输入和拖入文件。Ctrl+F 展开查找栏；菜单 View 切换换行及浅深主题。
- 新建、打开、关闭经过未保存确认；保存取消或失败时保留当前文档。异步写入保留保存快照，写入期间的新编辑继续显示未保存。
- 文件选择使用 FileDialog。文件 IO 是 Demo 逻辑，运行在 std::jthread，结果通过带弱生命周期的 Dispatcher 回到 UI；工作线程不访问 Widget。关闭清理停止后台工作，迟到结果不会访问已销毁对象。
- 仅处理最大 32 MiB 的 UTF-8 纯文本，接受 UTF-8 BOM，读取时统一换行为 LF；保存为无 BOM UTF-8 / LF。拒绝二进制、NUL、无效 UTF-8 和 UTF-16，避免静默改写未知编码。
- 保存先写入同目录独立临时文件，再替换目标；失败清理自己的临时文件。Windows 使用独占 CREATE_NEW 和 MoveFileExW，不提前截断原文档。系统存储故障与所有文件系统的断电持久性不在 Demo 承诺内。

## 数据管理

ListView / TreeView / TableView 同时使用一个 StandardItemModel 和多选 SelectionModel。初始示例包含可展开子项；所有视图共享名称、勾选及选择，表格显示状态和备注。

- Ctrl+N 添加、Ctrl+Delete 删除选中，F2 / 双击或菜单编辑；Enter 提交、Escape 取消。支持 Ctrl / Shift 多选、列标题排序及列宽调整。
- 数值框设置 1 到 100000 行，Ctrl+L 启动后台生成。每批最多 1000 行，UI 接收后才允许生成下一批，限制队列内存并给事件循环保留响应机会。
- 工作线程只创建数据，通过 Dispatcher 在 UI 线程修改模型；取消增加任务代数并请求停止，迟到批次丢弃，已加载行保留。关闭窗口后弱目标使回调失效。
- 分割布局可拖动及键盘调整；使用菜单、工具栏、工具提示、数值框、进度条、状态栏和右键菜单。

## 验证边界

文件编码、替换保存、取消清理及保存快照运行 CPU 测试。虚拟化、模型、选择、编辑和 Dispatcher 的基础行为在对应框架测试中覆盖。完整窗口流程、真实文件选择、输入法、无障碍朗读、视觉与 GPU 由用户按 [验收清单](ManualTesting.md) 检查；未将编译结果计为桌面验收。
