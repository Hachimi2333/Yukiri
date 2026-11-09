# Windows 无障碍

0.2 提供 Windows UI Automation 服务端，支持原生和自绘窗口；本轮仅运行语义与原生 COM 接口的无窗口测试，没有启动 Narrator 或 Inspect。实际朗读、系统事件及屏幕坐标验收见 [人工检查](ManualTesting.md)。

## 应用接口

`Widget::SetAccessibilityInfo(AccessibilityInfo)` 设置名称、说明、自动化标识、角色、标签关联及可见语义。`GetAccessibilityInfo()` 可覆盖；内置控件自动推断角色和可用模式。`m_Label` 是非拥有观察关系，目标销毁后不再读取。标签、图标按钮和密码字段仍应由应用提供清晰名称。

```cpp
Yukiri::AccessibilityInfo info;
info.m_Name = "Document text";
info.m_Description = "Plain text document editor";
info.m_AutomationId = "DocumentEditor";
editor.SetAccessibilityInfo(std::move(info));
```

应用线程采用 COM STA；Application 在未初始化 COM 时完成初始化及平衡清理。已经初始化为 MTA 的线程创建 Application 会返回错误，应将 UI 放到独立 STA。后台文件选择使用自己的 STA，不改变此约定。公共接口不暴露 COM。

## 语义与模式

- 控件树、窗口内弹出分支、模态层、Tab 标题和页面构成导航关系；模态出现时底层内容不可操作。
- 按控件提供 Invoke、Toggle、Selection / SelectionItem、ExpandCollapse、Value / RangeValue、Scroll / ScrollItem、Grid / Table、ItemContainer / VirtualizedItem、Window。
- TextBox / TextArea 提供 Text / Text2、字素 / 单词 / 可视行 / 段落范围、查找、选区、可见范围、坐标命中和滚入视口。格式是纯文本字体属性，不提供富文本和嵌入对象。
- 密码字段拒绝 Value 明文读取且不提供 Text 模式；之前持有的文字范围也立即失效于密码访问规则。应用自身的 GetText 不受影响。
- 列表 / 树 / 表格以稳定 ItemId 创建按需 Provider，不为十万行常驻创建节点或 Widget。Realize 展开祖先并滚入视口；模型替换使旧节点失效。
- 自绘系统按钮提供最小化、最大化 / 恢复及关闭操作，关闭经过应用可取消关闭流程。

## 生命周期与事件

平台适配拦截 WM_GETOBJECT，服务端采用应用 STA 的 COM 调度。访问经过与输入相同的分发保护，保留的 Provider 使用弱身份检查；窗口或控件销毁后返回元素不可用。读取失效对象不访问原始地址。

焦点、结构、值、选择、文本和状态变化以窗口更新为安全时点合并；位置换算到物理屏幕坐标。大文本值只按客户端订阅需要生成，热滚动不逐帧复制文档。关闭时断开 Provider，释放缓存及通知订阅。

## 验证边界

CPU 检查包括语义树、模态隔离、弱标签、密码保护、十万行虚拟数据，及直接调用原生 COM 接口的身份、模式、文字范围、稳定节点、跨线程拒绝与销毁行为。这些检查不创建 HWND、不连接桌面无障碍客户端，不代表 Narrator 已验收。

实现参考：[Microsoft 服务端 Provider](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-serversideprovider)、[文字与范围](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-implementingtextandtextrange)、[ProviderOptions](https://learn.microsoft.com/en-us/windows/win32/api/uiautomationcore/ne-uiautomationcore-provideroptions)。
