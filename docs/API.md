# API 使用

命名空间为 Yukiri。核心类型为 Application、Window、Widget、Canvas 和 Theme；配置对象使用 ApplicationConfig、WindowConfig。

WindowDecoration 提供 Native / Custom，默认为 Custom，创建时确定。ThemeMode 提供 System / Light / Dark，默认跟随系统。

Application::Create 返回 expected<unique_ptr<Application>, Error>；CreateWindow 返回由 Application 拥有的 Window 观察指针，窗口关闭并完成下一次 Poll 后该指针失效；保存返回的 Connection 以监听 m_Closed，并在回调内清空外部观察指针。Run 驱动主循环，Poll 可用于外部循环。所有窗口 / UI API 在创建 Application 的线程调用。Poll 不可重入；失败返回 ErrorCode::InvalidArgument。

Close 合并关闭请求，在下一次 Poll 的安全边界发送 m_CloseRequested。回调可设置 CloseRequestEvent::m_Cancel=true 暂缓关闭，例如异步保存确认；用户确认后再次调用 Close。系统关闭和 Quit 使用相同规则。IsOpen 在请求接受后变为 false，m_Closed 在实际释放之前仅发送一次。创建窗口的配置先校验尺寸；m_Visible=false 可用于用户启动的 GPU smoke。

## 控件与布局

Window::SetContent 接管根控件。使用 AddChild<T> 创建并添加子控件，返回引用；Keep 保存 Signal::Connect 返回的 Connection，连接对象离开作用域会断开。RemoveChild / ClearChildren 在输入、布局和绘制分发期间延迟执行。

Measure 的 desired size 包含 margin；Arrange 接收全局逻辑坐标的槽位，扣除 margin 后按尺寸约束与 alignment 排列。OnArrange 的内容矩形也使用全局逻辑坐标；OnPaint 的 Canvas 已平移到控件局部坐标。

StackPanel 在主轴按自然尺寸排列，Grid 支持固定 / Auto / 加权 Star 及 span；SetCell 返回无效单元格错误。Overlay 按子控件顺序叠放，逆序命中测试。焦点在窗口内循环，禁用或隐藏的子树不会获得焦点。

## 基础控件

可包含 Yukiri/Yukiri.hpp，或单独包含所需头文件。所有公共头文件都以独立翻译单元进行构建检查。

Button 提供 m_Clicked，CheckBox / ToggleSwitch / RadioButton 提供 m_CheckedChanged。RadioButton 在同一父级、相同 group 内互斥。Slider 提供范围与 m_ValueChanged，ProgressBar 使用 0..1 进度值。

ComboBox 是不可编辑下拉框，SetSelectedIndex 返回范围检查结果；支持方向键、Home / End、Enter 和 Escape，内容在窗口内弹出层绘制。TabView 使用 AddTab 管理页面，切换后隐藏非活动子树。

ScrollView 使用 SetContent，默认纵向滚动，可启用横向滚动；支持滚轮、拖动滚动条和焦点控件自动进入视口。复杂控件的结构内容应通过 SetContent / AddTab 管理。

## 窗口装饰

WindowConfig::m_Decoration 在创建时选择 Native / Custom，默认 Custom。自绘模式绘制标题与标准系统按钮，客户根控件在标题栏下排列。窗口支持 SetSize、Minimize、Maximize 和 Restore，可失败操作返回 expected。

Windows 宿主程序可在自己的 EXE 中链接图标资源，原生窗口及自绘标题栏使用该应用图标；没有图标时，自绘标题栏保持文字布局。Yukiri 标识仅链接到 Demo，不作为消费应用的默认图标。本次未增加公开图标设置 API。

公共接口不用包含 Windows.h。与 Win32 混用时使用 CreateWindowW / DrawTextW 等显式名称；Yukiri 头文件会移除同名宏别名，避免覆盖 C++ 成员名称。

## 最小应用

链接 Yukiri::Yukiri 后，可编写以下 Main.cpp；自有源文件使用 UTF-8：

```cpp
#include <Yukiri/Yukiri.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

int main()
{
	auto created = Yukiri::Application::Create();
	if (!created)
	{
		std::cerr << created.error().m_Message << '\n';
		return 1;
	}
	auto app = std::move(*created);
	auto opened = app->CreateWindow();
	if (!opened)
	{
		std::cerr << opened.error().m_Message << '\n';
		return 2;
	}
	auto root = std::make_unique<Yukiri::StackPanel>();
	root->SetPadding({24, 24, 24, 24});
	auto& label = root->AddChild<Yukiri::Label>("Count: 0");
	auto& button = root->AddChild<Yukiri::Button>("Click me");
	button.Keep(button.m_Clicked.Connect([&label, count = 0]() mutable
	{
		label.SetText("Count: " + std::to_string(++count));
	}));
	(*opened)->SetContent(std::move(root));
	auto result = app->Run();
	if (!result) std::cerr << result.error().m_Message << '\n';
	return result ? 0 : 3;
}
```

自定义控件继承 Widget，并重写 OnMeasure、OnArrange 或 OnPaint；OnPaint 接收局部坐标 Canvas。逻辑尺寸使用 DIP，Window::GetScale 返回实际缩放，GetContentBounds 返回排除自绘标题栏的全局逻辑内容区域。

Signal::Connect 可带弱生命周期保护；Connection 析构自动断开。回调内允许断开连接或触发其他信号，当前分发使用连接快照。不要在回调内析构正在执行的 Application。

## 调度、命令与窗口服务

Application::GetDispatcher 返回可复制的非拥有调度句柄。后台线程可以 Post 移动专用闭包，或使用弱 owner 重载；闭包只在应用线程执行。一次 Poll 至多执行 256 个已排队任务，回调中新投递的任务留到下一轮。窗口最终退出后以及 Application 析构后，Post 返回错误，不再接受工作；应用应在退出前停止并汇合自己拥有的后台线程。

Dispatcher::CreateTimer 在应用线程创建正间隔定时器，返回移动专用的 Timer。Timer 析构或 Stop 取消后续回调；单次定时器只执行一次，重复定时器落后时不补发历史 tick。回调允许停止或销毁自己的 Timer。定时器不创建线程，事件等待同时考虑其截止时间。任务回调与其他 UI 回调一样，不应抛出异常或递归驱动 Poll。

Command 使用 m_Executed 执行动作，m_Changed 通知文字、启用、勾选或快捷键变化。Command 的勾选状态由应用更新，不自动反转。Button::SetCommand 绑定文字与启用状态；绑定期间应用应通过 Command 设置这两项。Widget、Window 和 Application 的 AddCommand / RemoveCommand 管理相应快捷键作用域。焦点控件先处理编辑按键，再由祖先、窗口和应用命令处理；弹出层优先，Tab 导航最后执行。默认不重复执行按住的快捷键，可用 SetAllowRepeat 开启。AltGr 不触发命令。

Window 提供 Show / Hide / Activate / SetOwner，所属关系禁止成环；所有权观察指针在所属窗口关闭时清空。m_SizeChanged / m_StateChanged 报告平台窗口通知，m_FilesDropped 提供一批 filesystem::path。真实平台行为需要用户启动桌面验收。按钮 Space 在释放时激活，Enter 在按下时激活；窗口重新获得焦点时恢复仍然有效的最近焦点，但不主动显示键盘焦点框。

## 字体、文字与自适应布局

Font 描述 UTF-8 字体族、100..900 字重（默认 400）及斜体，空字体族沿用默认回退。Theme::m_Font 提供主题默认，WidgetStyle::m_Font 提供整个字体描述的局部覆盖；字体描述沿父控件继承，字号继续由既有 m_FontSize 控制。不存在的字体回退到系统链。Windows 按实际字体族 / 字重 / 斜体查找文件，变量字体设置相应轴，缺少样式时合成粗体 / 斜体。测量、绘制、文本框光标及命中共享字体描述；原有无 Font 参数调用继续有效。

Label 支持 SetWrap、SetEllipsis、SetTextAlignment；省略号用于非换行文本，换行模式保留显式换行并按 Unicode 单词边界选择断点，过长单词按字素拆分。Border 接管一个内容控件，默认 12 DIP 内边距。WrapPanel 按方向换行并跳过隐藏子控件。SplitPanel::SetContent 需要两个非空内容，支持方向、比例、最小尺寸、键盘方向键 / Home / End、Shift 精细调整及直接拖动；空间不足时按最小尺寸比例压缩。结构内容通过 SetContent 管理，不直接移除其内部子控件。

Button 和 Command 可使用 SetIcon 提供 Image；命令绑定同步按钮图标，按钮文字在窄宽度下使用字素安全的省略号。普通按钮显示命令的勾选状态，动作负责修改状态；CheckBox / ToggleSwitch / RadioButton 绑定可勾选命令后会同步状态，用户切换控件时先更新该状态再执行命令。

## 菜单、提示与对话框

MenuBar::SetItems 接受 ContextMenu::Item 的层级数组。Item::Action 绑定共享 Command，Submenu 提供标题及子项，Divider 提供分隔线；顶层标题的 & 标记 ASCII 助记键，&& 表示字面 &。菜单栏自动连接窗口快捷键，F10 或 Alt+助记键进入菜单。菜单支持上下 / Home / End / PageUp / PageDown、左右子菜单、Enter / Space 执行和 Escape 逐层退出；滚轮可访问高于窗口的菜单。图标、启用、勾选和快捷键文本来自 Command。命令勾选仍由应用更新。

ContextMenu::Show(owner, items, localPosition) 在所属窗口打开上下文菜单；Bind(owner, items) 同时绑定右键释放、菜单键及 Shift+F10，位置使用 owner 的局部坐标。MenuBar 和 ContextMenu 的子项是数据，菜单不为每一行创建常驻 Widget。ToolBar / StatusBar 是带主题背景的布局容器；Separator 支持水平 / 垂直方向。Widget::SetToolTip 设置 500ms 悬停提示，提示不抢焦点，按下、滚动、失焦和销毁时撤销。

Dialog::Show(window, unique_ptr<Dialog>) 把所有权交给窗口模态层，返回非拥有观察指针；SetContent 提供内容，SetButtons 配置结果按钮。m_Completed 回调异步结果式完成，不嵌套调用 Poll；m_CloseRequested 可以否决正常完成。Escape 使用取消结果，Enter 使用默认按钮，但保留按钮、ComboBox 和多行编辑器本身的 Enter 语义；IME 组合期间按键留给编辑器。关闭窗口强制取消模态内容。MessageDialog::Show 提供常见按钮组合。

FileDialog::Open / Save / SelectFolder 接受 Window、Config 和回调，立即返回启动结果；回调在 UI 线程收到 Result<vector<filesystem::path>>。空成功数组表示用户取消，unexpected 表示错误。Config 包括标题、默认路径、过滤器（如 *.txt;*.md）、默认扩展名与多选；保存不允许多选。每个所属窗口同时只允许一个请求。Cancel 请求取消；窗口确定关闭或应用析构时取消并清理工作线程，丢弃迟到结果。Windows IFileDialog 在独立 STA 运行，框架不改变进程工作目录；关闭清理只泵送必要的原生消息，不重入 Yukiri::Poll。

## 图片

Image 是可复制的共享不可变资源，Load 接受 filesystem::path，FromMemory 接受编码字节，FromRgba 接受精确的 width * height * 4 字节。GetPixels 返回资源持有期间有效的直通 Alpha RGBA8 视图；SVG 的该视图为原始尺寸栅格结果，绘制时另按 DPI 重栅格化。支持 PNG / JPEG / BMP / ICO 与静态 SVG，不支持动画图像；SVG 使用自包含文档，不承诺外部资源或完整浏览器 SVG 特性。

ImageView 的 Stretch::None / Uniform / UniformToFill / Fill 分别表示原尺寸、等比适应、等比填充裁剪及拉伸。Canvas::DrawImage(image, rectangle, opacity) 绘制到局部矩形并遵循当前裁剪。CPU 图片可独立于窗口加载；GPU 资源只在应用线程绘制时创建。编码输入上限 64MiB，尺寸每轴最多 16384，解码像素最多 64M（256MiB RGBA）；栅格格式在调用解码器前检查尺寸，GPU 另检查设备限制。

## 文字编辑与数值输入

TextArea 继承 TextBox 的文本、只读、选区、剪贴板、撤销和 IME 接口；默认自动换行，SetWrap(false) 开启横向滚动。支持上下 / Home / End / PageUp / PageDown、Ctrl+Home / End、Shift 扩选、双击选词、三击选逻辑行及越界拖动滚动。Tab 继续参与控件导航。SetSelection 的参数是 UTF-8 字节偏移并向前校正到字素边界。SetOffset 立即定位，滚轮使用主题滚动时长缓动。

Find、Replace、ReplaceAll 提供字面查找替换及大小写选项；Find 默认从当前选区之后查找并循环，backwards 从选区之前向前查找。Replace 优先替换当前匹配选区，否则查找下一个；ReplaceAll 返回替换数量并只产生一项撤销记录。Undo / Redo、CanUndo / CanRedo、m_EditStateChanged 便于绑定编辑命令；Copy / Cut / Paste 返回操作结果，取消预编辑后操作已提交文本。TextBox::SetPassword 只作用于单行，屏蔽复制 / 剪切和无障碍明文；应用的 GetText 仍返回实际值。

NumberBox 通过 GetEditor 提供文本输入，SetRange / SetStep / SetFormat 检查范围、步长及 General / Fixed / Scientific 格式精度。输入使用不依赖区域的十进制与小数点，可包含指数；非数字、非有限值和越界值不提交。Enter / 失焦尝试提交，Escape 恢复已提交值，Up / Down 增减一步，PageUp / PageDown 增减十步。无效内容显示边框反馈，GetValue 保留已提交值。程序 SetValue 约束到范围；格式舍入会越界时使用足够精度保留合法端点。

单选组以同级相同 group 标识组织，Tab 进入已选项（未选时进入首个有效项），方向键循环跳过隐藏 / 禁用项并选择。Slider::SetStep 设置键盘步长及拖动吸附，Home / End 到端点，Shift 或 PageUp / PageDown 扩大增量。ComboBox 关闭时方向键 / Home / End 直接选择，Alt+Down / F4 / Space / Enter 展开；列表支持 PageUp / PageDown，Escape / F4 / Alt+Up 退出。ProgressBar::SetIndeterminate 显示循环进度，禁用动效时显示静态标记。

## 数据模型与视图

数据接口和 ListView / TreeView / TableView 的使用见 [数据模型文档](Data.md)。模型以稳定 ID 共享，选择可以在多视图之间复用，编辑器按需创建。

## 动画与平滑滚动

ApplicationConfig::m_Motion 设置默认动效，Theme::m_Motion 支持局部覆盖。使用 m_Enabled=false 可整体关闭该作用域的动效；非正时长立即完成。初始属性直接显示，后续状态变化使用短过渡，模型 getter 与通知始终立即反映目标值。

ScrollView 默认平滑处理滚轮与现有方向键 / Home / End，支持小数滚轮增量、Shift 横向滚动和嵌套边界传递。GetOffset 返回当前可见位置；SetOffset 立即定位并取消尚未完成的滚动。SetSmoothScrolling(false) 立即完成当前目标，后续输入直接移动。滚动条拖动和焦点进入视口始终立即完成。

指针点击不会因获取焦点而自动移动当前视口；键盘 Tab 导航和 RequestFocus 仍使焦点控件可见。纯滚动平移子树的全局坐标，命中测试、裁剪与 IME 光标区域保持一致，不触发整棵树重新测量。

ComboBox 退出动画期间已不再接收下拉输入。窗口变化、焦点丢失、滚动或所属控件销毁会直接撤销弹出层。自定义控件可使用带 progress 参数的 ShowPopup 绘制回调；回调与窗口内弹出层共享安全生命周期。

键盘焦点显示与输入焦点分开：HasFocus() 表示实际输入焦点，IsFocusVisible() 仅在 Tab / Shift+Tab 导航模式且当前控件有焦点时返回 true。鼠标按下（含下拉层）隐藏焦点提示；移动、滚轮和文字输入不改变模式。RequestFocus() 继承窗口模式。自定义控件重写受保护的 GetFocusBounds() 返回局部焦点区域，并使用 PaintFocus() 绘制；键盘获取焦点按该区域滚入视口，显式 BringIntoView() 仍使用整个控件范围。

## Windows UI Automation

使用 Widget::SetAccessibilityInfo 设置应用名称、说明、AutomationId 或标签关联。内置控件自动暴露对应模式，密码字段禁止无障碍明文；自定义控件可覆盖 GetAccessibilityInfo。应用线程需要 COM STA，后台线程通过 Dispatcher 投递 UI 操作。详细模式、虚拟节点和验收边界见 [无障碍](Accessibility.md)。
