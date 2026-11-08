# 渲染

目标为 Vulkan 1.4，使用 volk、VMA、Dynamic Rendering 和 Synchronization2。GLSL 在构建时编译为嵌入式 SPIR-V，应用运行时不需要着色器编译器。

一个应用共享 Vulkan 设备与 VMA allocator。每个窗口独立拥有交换链、管线和两个帧槽；每槽拥有命令缓冲、顶点缓冲、acquire semaphore 和 fence，present semaphore 按交换链图像分配，避免错误复用。

只选择具备呈现能力的 Vulkan 1.4 图形队列，启用 dynamicRendering 和 synchronization2。使用 sRGB 交换链、线性空间颜色和预乘 Alpha。矩形 / 圆角通过解析距离场抗锯齿，文字使用 R8 图集。按有序批次绑定纹理与矩形裁剪，保留透明叠放顺序。

交换链尺寸变化和 out-of-date 在安全等待后重建，最小化或零尺寸不提交。GPU 资源在所属窗口与设备销毁之前释放。图集更新使用暂存缓冲与同步上传，普通帧使用独立映射顶点缓冲。

当前已完成双窗口清屏和呈现的实际 GPU 验证；复杂绘制效果随 Demo 验证。
