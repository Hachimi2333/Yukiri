# 渲染

运行基线为 Vulkan 1.4，使用 volk、VMA、Dynamic Rendering 和 Synchronization2。GLSL 在构建时编译为嵌入式 SPIR-V；运行时不需要着色器编译器或 SDK。

应用共享 Vulkan 设备与 VMA allocator。每个窗口独立拥有交换链、管线和两个帧槽；每槽保存命令缓冲、顶点缓冲、acquire semaphore 和 fence。present semaphore 按交换链图像分配，避免错误复用。

选择支持呈现的 Vulkan 1.4 图形队列，启用 dynamicRendering 和 synchronization2。使用 sRGB 交换链、线性空间颜色和预乘 Alpha。矩形 / 圆角使用解析距离场抗锯齿，文字使用 R8 图集。绘制列表按原始顺序合并相邻纹理 / 裁剪批次，保留透明叠放顺序。Canvas 提供矩形、边框、圆角、线条、文字、平移与裁剪栈。

字形使用实际像素字号栅格化，四边形落点对齐像素网格，减少分数 DPI 下重复采样带来的模糊。图集更新使用暂存缓冲和同步上传，普通帧使用映射顶点缓冲。

交换链尺寸变化、out-of-date 和 suboptimal 在安全等待后重建，并保留重绘请求。最小化或零尺寸不提交。窗口资源先于共享设备释放；设备销毁前等待 GPU 完成。

`YUKIRI_TRACE_FRAMES=1` 在窗口渲染器销毁时打印提交帧数。配合 Demo 的 `--idle-smoke` 可检查空闲窗口不连续重绘。实际 GPU、验证层与帧数结果见 [验证记录](Validation.md)。
