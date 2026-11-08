# API 使用

命名空间为 Yukiri。核心类型为 Application、Window、Widget、Canvas 和 Theme；配置对象使用 ApplicationConfig、WindowConfig。

WindowDecoration 提供 Native / Custom，默认为 Native，创建时确定。ThemeMode 提供 System / Light / Dark，默认跟随系统。

Application::Create 返回 expected<unique_ptr<Application>, Error>；CreateWindow 返回由 Application 拥有的 Window 观察指针，窗口关闭并完成下一次 Poll 后该指针失效。Run 驱动主循环，Poll 可用于外部循环。所有窗口 / UI API 在创建 Application 的线程调用。

Close 只标记关闭，实际销毁在事件处理的安全边界完成。创建窗口的配置先校验尺寸；m_Visible=false 可用于自动化 smoke。

## 控件与布局

Window::SetContent 接管根控件。使用 AddChild<T> 创建并添加子控件，返回引用；Keep 保存 Signal::Connect 返回的 Connection，连接对象离开作用域会断开。RemoveChild / ClearChildren 在输入分发期间延迟执行。

Measure 的 desired size 包含 margin；Arrange 接收全局逻辑坐标的槽位，扣除 margin 后按尺寸约束与 alignment 排列。OnArrange 的内容矩形也使用全局逻辑坐标；OnPaint 的 Canvas 已平移到控件局部坐标。

StackPanel 在主轴按自然尺寸排列，Grid 支持固定 / Auto / 加权 Star 及 span；SetCell 返回无效单元格错误。Overlay 按子控件顺序叠放，逆序命中测试。焦点在窗口内循环，禁用或隐藏的子树不会获得焦点。
