# 设计决策

本文件记录与用户确认的首版决策。

- 通用桌面应用，保留模式控件树，SDL3 负责窗口与系统输入。
- C++23、CMake >= 3.28、Ninja；Windows 10 22H2 / Windows 11 x64，MSVC 与 MinGW。
- Vulkan 运行基线 1.4，C API + volk + VMA，GLSL 构建时编译。
- C++ 容器布局和类型化样式，不引入标记语言或 CSS 解析器。
- FreeType + HarfBuzz + libunibreak，先覆盖中英文；Catch2 + CTest。
- 父级独占子级，Signal / Connection 管理通知，expected + Error / ErrorCode 报告失败。
- 核心命名空间 Yukiri，配置后缀 Config，构建目标 Yukiri::Yukiri，程序 YukiriDemo。
- 多窗口，原生与自绘标题栏均支持，框架与 Demo 默认自绘（2026-11-08 调整）；浅深主题默认跟随系统。
- 自绘标题栏包含常用 Windows 行为与 Windows 11 Snap Layouts。
- 提供源码集成与可重定位安装包，第三方静态依赖由 CMake 目标传递。

- 键盘焦点只由 Tab 导航开启，鼠标点击隐藏；选择类控件使用图标与文字紧凑区域，TabView 使用当前标题区域。
- 输入法组合文本由框架显示下划线与组合选区，候选列表由系统提供；实际组合内容以输入法事件为准。
- 项目标识仅用于 README 与 Demo；消费应用不自动携带 Yukiri 品牌。Windows 自绘标题栏通用地显示宿主 EXE 的图标资源，不新增公开图标 API 或运行时 SVG 依赖。
- 不配置 GitHub Actions，构建与验证由本机工具完成。

- 0.2 补充：公共图片使用 SDL_image 3.4.8 与 LunaSVG 3.5.0 的静态源码依赖；0.1 图标阶段没有增加 SVG 解析依赖的记录属于该阶段历史。框架依然不包含 Yukiri 品牌图像，宿主图标接口仍为内部实现。
