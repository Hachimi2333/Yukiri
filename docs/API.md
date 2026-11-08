# API 使用

命名空间为 Yukiri。核心类型为 Application、Window、Widget、Canvas 和 Theme；配置对象使用 ApplicationConfig、WindowConfig。

WindowDecoration 提供 Native / Custom，默认为 Native，创建时确定。ThemeMode 提供 System / Light / Dark，默认跟随系统。

Application::Create 返回 expected<unique_ptr<Application>, Error>；CreateWindow 返回由 Application 拥有的 Window 观察指针，窗口关闭并完成下一次 Poll 后该指针失效。Run 驱动主循环，Poll 可用于外部循环。所有窗口 / UI API 在创建 Application 的线程调用。

Close 只标记关闭，实际销毁在事件处理的安全边界完成。创建窗口的配置先校验尺寸；m_Visible=false 可用于自动化 smoke。
