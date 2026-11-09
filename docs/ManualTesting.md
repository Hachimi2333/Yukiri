# 用户启动的桌面验收

本轮开发不使用 Computer Use，不代替用户操作桌面。以下检查由用户有空时启动。
未执行的项目保持待验收，编译与 CPU 测试不替代真实体验。

## 运行和日志

使用 `tools/Build.ps1 -Preset msvc-release -Test` 构建后，运行 `out/build/msvc-release/YukiriDemo.exe`。
GPU 检查可自行运行 `tools/GpuSmoke.ps1 -Preset msvc-release`，日志写入 out 下对应 preset 的 stdout / stderr 文件。
也可用 MinGW 对应 preset 验证；本轮自动工作不启动这些 GPU 程序。

## 应用、输入与窗口

- [ ] 原生 / 自绘窗口的点击、双击、拖动和系统按钮正常。
- [ ] Tab / Shift+Tab 显示焦点框，鼠标点击隐藏焦点框。
- [ ] 按住 Space 时按钮显示按下，释放激活一次；切到其他应用再释放不误激活。
- [ ] 拖动滑块或滚动条时切到其他应用，回来后没有残留拖动状态。
- [ ] 中英文编辑、AltGr 输入和应用快捷键互不抢占。
- [ ] 文件拖入程序后得到正确的一批 Unicode 路径。
- [ ] 可取消关闭、所属窗口、Show / Hide / Activate 在实际应用流程中行为正常。

菜单、多行编辑、数据管理、图片和无障碍检查会随对应阶段交付补充。
