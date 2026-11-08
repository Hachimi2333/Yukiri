# 样式

使用类型化 C++ 主题与样式，提供浅色、深色和跟随系统模式。视觉采用中性色、蓝色强调、小幅圆角、清晰焦点框与克制的状态变化。

Theme 保存颜色与字号 / 圆角 / 高度等设计参数。Window::SetTheme 选择 System / Light / Dark；系统主题变化时，System 窗口更新客户区与可用的原生标题栏深色属性。

WidgetStyle 提供类型化可选覆盖，SetThemeOverride 可在子树替换 Theme。子控件继承最近的主题。悬停颜色用 120ms 过渡，按下和焦点立即反馈；动画结束后停止逐帧调度，空闲等待事件。

自定义控件可复用 InteractiveColor、PaintFocus 和 Canvas，保持状态与主题一致。
