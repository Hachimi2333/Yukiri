# 依赖

所有第三方库均以 Git submodule 引入并固定到具体提交。运行库依赖静态构建，Catch2 只用于测试。Vulkan SDK 提供头文件、着色器工具和开发验证层，运行时通过 volk 加载系统 Vulkan 驱动。

| 依赖 | 标签 | 许可证 |
|---|---|---|
| SDL3 | release-3.4.20 | zlib |
| volk | vulkan-sdk-1.4.363.0 | MIT |
| VulkanMemoryAllocator | v3.5.0 | MIT |
| FreeType | VER-2-15-0 | FreeType License |
| HarfBuzz | 15.1.0 | MIT / 组件原始声明 |
| libunibreak | libunibreak_8_0 | zlib / Unicode 数据许可 |
| Catch2 | v3.16.1 | BSL-1.0 |

精确提交由 Git gitlink 固定，可用 git submodule status 检查。FreeType 禁用可选压缩、PNG、Brotli 和内置 HarfBuzz 集成，HarfBuzz 单向依赖 FreeType，避免循环构建。HarfBuzz 的 raster / vector / GPU 子库不参与构建。SDL 仅保留窗口、事件和 Vulkan 相关能力。libunibreak 使用项目维护的 CMake target 编译上游列出的源文件，不改写上游。

VMA 实现编译进 Yukiri，不单独形成运行时 DLL。
