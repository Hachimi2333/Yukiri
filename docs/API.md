# API 使用

命名空间为 Yukiri。核心类型为 Application、Window、Widget、Canvas 和 Theme；配置对象使用 ApplicationConfig、WindowConfig。

WindowDecoration 提供 Native / Custom，默认为 Native，创建时确定。ThemeMode 提供 System / Light / Dark，默认跟随系统。

Application::Create 返回 expected<unique_ptr<Application>, Error>；CreateWindow 返回由 Application 拥有的 Window 观察指针，窗口关闭并完成下一次 Poll 后该指针失效；保存返回的 Connection 以监听 m_Closed，并在回调内清空外部观察指针。Run 驱动主循环，Poll 可用于外部循环。所有窗口 / UI API 在创建 Application 的线程调用。Poll 不可重入；失败返回 ErrorCode::InvalidArgument。

Close 只标记关闭，实际销毁在事件处理的安全边界完成。创建窗口的配置先校验尺寸；m_Visible=false 可用于自动化 smoke。

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

WindowConfig::m_Decoration 在创建时选择 Native / Custom，默认 Native。自绘模式绘制标题与标准系统按钮，客户根控件在标题栏下排列。窗口支持 SetSize、Minimize、Maximize 和 Restore，可失败操作返回 expected。

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

## 动画与平滑滚动

ApplicationConfig::m_Motion 设置默认动效，Theme::m_Motion 支持局部覆盖。使用 m_Enabled=false 可整体关闭该作用域的动效；非正时长立即完成。初始属性直接显示，后续状态变化使用短过渡，模型 getter 与通知始终立即反映目标值。

ScrollView 默认平滑处理滚轮与现有方向键 / Home / End，支持小数滚轮增量、Shift 横向滚动和嵌套边界传递。GetOffset 返回当前可见位置；SetOffset 立即定位并取消尚未完成的滚动。SetSmoothScrolling(false) 立即完成当前目标，后续输入直接移动。滚动条拖动和焦点进入视口始终立即完成。

指针点击不会因获取焦点而自动移动当前视口；键盘 Tab 导航和 RequestFocus 仍使焦点控件可见。纯滚动平移子树的全局坐标，命中测试、裁剪与 IME 光标区域保持一致，不触发整棵树重新测量。

ComboBox 退出动画期间已不再接收下拉输入。窗口变化、焦点丢失、滚动或所属控件销毁会直接撤销弹出层。自定义控件可使用带 progress 参数的 ShowPopup 绘制回调；回调与窗口内弹出层共享安全生命周期。

键盘焦点显示与输入焦点分开：HasFocus() 表示实际输入焦点，IsFocusVisible() 仅在 Tab / Shift+Tab 导航模式且当前控件有焦点时返回 true。鼠标按下（含下拉层）隐藏焦点提示；移动、滚轮和文字输入不改变模式。RequestFocus() 继承窗口模式。自定义控件重写受保护的 GetFocusBounds() 返回局部焦点区域，并使用 PaintFocus() 绘制；键盘获取焦点按该区域滚入视口，显式 BringIntoView() 仍使用整个控件范围。
