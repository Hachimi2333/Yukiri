# 样式

使用类型化 C++ 主题与样式，提供浅色、深色和跟随系统模式。视觉采用中性色、蓝色强调、小幅圆角、清晰焦点框与克制的状态变化。

Theme 保存颜色与字号 / 圆角 / 高度等设计参数。Window::SetTheme 选择 System / Light / Dark；系统主题变化时，System 窗口更新客户区与可用的原生标题栏深色属性。

WidgetStyle 提供类型化可选覆盖，SetThemeOverride 可在子树替换 Theme。子控件继承最近的主题。悬停颜色用 120ms 过渡，按下和焦点立即反馈；动画结束后停止逐帧调度，空闲等待事件。

自定义控件可复用 InteractiveColor、PaintFocus 和 Canvas，保持状态与主题一致。

## 动画配置

MotionSettings 通过 ApplicationConfig::m_Motion 设置应用默认值，创建窗口后由 Theme::m_Motion 继承，局部主题可覆盖。状态、选择、弹出层和滚动默认分别为 120 / 160 / 120 / 140ms；m_Enabled=false 或非正时长立即到达目标。切换浅深主题不丢失应用动效设置。

自定义控件重写受保护的 bool OnAnimate(std::chrono::nanoseconds delta)，更新显示状态并返回是否继续；状态变化时调用 RequestAnimationFrame。回调位于延迟销毁保护范围。控件模型值与信号应立即更新，动画仅改变显示。内部 Transition 使用 cubic ease-out，并从当前显示值衔接新的目标。

内置控件动效包括开关轨道 / 圆点、复选框 / 单选标记、按钮按下 / 悬停、下拉箭头 / 展开 / 淡入淡出 / 选项高亮、Tab 指示条 / 背景，以及滑块与进度变化。Tab 内容即时切换；滑块拖动直接跟手。焦点框及时显示，不等待过渡完成。
