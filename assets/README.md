# Yukiri 图标

`Yukiri.svg` 是用户提供的原始标识，按字节原样保存。SVG 是图标源文件，README 直接引用它。

`Yukiri.ico` 包含 16、20、24、32、40、48、64、128、256 像素的透明 PNG 图像，仅通过 Windows RC 资源链接到 YukiriDemo。框架静态库和消费项目不包含 Yukiri 品牌资源。

ICO 在开发时从 SVG 离线栅格化生成（sharp 0.35.5 / librsvg 2.63.2），不是常规构建步骤；构建不需要 Node.js、sharp 或 SVG 解析库。更新标识时，应从 SVG 重新导出以上尺寸，按 ICO 格式封装，并检查小尺寸、透明边缘和浅深标题栏。

图标遵循仓库的 MIT 许可证。安装文档保留 SVG 和 ICO，Demo 的运行不依赖外置图标文件。
