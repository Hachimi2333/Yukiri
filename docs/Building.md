# 构建与集成

需要 CMake >= 3.28、Ninja、支持 C++23 的 MSVC 或 MinGW，以及 Vulkan SDK 1.4。首轮验证环境包含 SDK 1.4.363.1。

构建命令将在 CMake 阶段加入。MSVC 与 MinGW 必须分别构建全部依赖，不能混用对象文件或静态库。
