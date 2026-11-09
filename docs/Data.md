# 数据模型与虚拟化视图

ItemModel 使用 ItemId（uint64_t）表示稳定的数据行，0 是虚拟根且不表示可选择的行。行可以包含子行和多列 Cell；Cell 提供文本、Image、启用、可选择、文本可编辑及可选的勾选状态。没有 m_Checked 表示非勾选单元格，false 表示可勾选但未选中。

应用可以继承 ItemModel 连接数据源，也可以使用 StandardItemModel 的 AppendRow / AppendRows / InsertRow / Remove / Move / SetCell。批量增加用 AppendRows，只发出一批插入通知。StandardItemModel 检查父关系、列范围和成环移动，Clear 后不重复使用旧 ID。SetHeaders 至少保留一列，并作为重置处理。基本排序按 UTF-8 文本顺序稳定排序各级同胞；排序后插入及修改排序列会重新维持顺序。需要数值、区域或业务排序时由自定义模型实现 Sort。

模型与 SelectionModel 的修改在创建它们的 UI 线程执行，后台任务通过 Dispatcher 投递。自定义模型在数据修改完成后用 Notify 发出 Change；Inserted / Removed 的 m_Parent、m_First、m_Count 描述结构区间，m_Items 包含受影响 ID（删除时包含所有已删除子孙）；Updated 的 m_Items 包含改变的行，Reordered 保留 ID，Reset 表示选择和编辑状态必须清理。模型必须始终是无环的树，同一父级不会重复返回同一 ID，读取接口不得在内部修改结构。

SelectionModel 与数据共享，不拥有 Widget。Mode::Single / Multiple 决定选择规则；SetSelected / Select / SetCurrent 可以从应用控制选中行与当前行。ListView / TreeView / TableView 通过 SetModel 接受 shared_ptr，通过 SetSelectionModel 共享同一模型的选择。切换视图模型会创建新的选择模型并保留模式，避免修改其他视图仍在使用的选择。排序与插入按 ID 保留选择，删除清理已失效 ID，Reset 清空选择。

三个视图使用统一行高和行索引，不为每行创建 Widget；只有可见行生成绘制数据。结构变化时重建行索引，热滚动不重新读取全部模型结构。TreeView 根据展开状态维护行索引，ScrollIntoView 自动展开所属路径。GetVisibleRange 返回当前视口内的半开行号范围，GetItemAt / GetRowIndex 在行号和 ID 之间转换，GetItemBounds 返回经过当前视口裁剪的窗口逻辑坐标。

支持鼠标单选、Ctrl 切换、Shift 范围、Ctrl+A、方向键 / Home / End / PageUp / PageDown 和平滑滚动；树使用 Left / Right 折叠、展开及进入子行。表格支持拖动列边界、点击标题排序和 Left / Right 移动当前列，行选择与当前列分开保存。GetCurrentColumn / SetCurrentColumn 可查询和指定当前列；F2 编辑当前单元格，双击可编辑文本也会打开编辑器。

BeginEdit 按需创建一个 TextBox，Enter 或失焦尝试提交，Escape 取消。模型 SetText 拒绝时保留内容、边框和错误提示；模型重置、删除行、外部修改编辑行或替换模型时清理旧编辑器。提交回调中替换模型或开启新编辑器不会被旧提交误关闭。勾选单元格支持鼠标与 Space，Ctrl+Space 用于切换当前行选择。只读模型通过拒绝 SetText / SetChecked 报告错误。

首版不包括可变行高、单元格范围选择、富文本单元格、电子表格公式、跨视图拖动排序和跨应用拖放。绘制及真实桌面体验由用户验收；十万行测试只证明 CPU 数据访问与 Widget 数量的约束，不代表 GPU 性能承诺。
