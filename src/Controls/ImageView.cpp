#include <Yukiri/Controls/ImageView.hpp>
#include <Yukiri/Render/Canvas.hpp>
#include <algorithm>
#include <cmath>

namespace Yukiri
{
ImageView::ImageView(Image image) : m_Image(std::move(image)) {}
void ImageView::SetImage(Image image) { if (m_Image == image) return; m_Image = std::move(image); InvalidateLayout(); }
const Image& ImageView::GetImage() const noexcept { return m_Image; }
void ImageView::SetStretch(Stretch stretch) { m_Stretch = stretch; InvalidateLayout(); }
Size ImageView::OnMeasure(Size available)
{
	auto size = m_Image.GetSize();
	if (size.m_Width <= 0 || size.m_Height <= 0 || m_Stretch == Stretch::None) return size;
	float ratio = 1;
	if (std::isfinite(available.m_Width)) ratio = std::min(ratio, available.m_Width / size.m_Width);
	if (std::isfinite(available.m_Height)) ratio = std::min(ratio, available.m_Height / size.m_Height);
	return {size.m_Width * ratio, size.m_Height * ratio};
}
void ImageView::OnPaint(Canvas& canvas)
{
	if (!m_Image) return;
	const auto source = m_Image.GetSize();
	const auto size = GetActualSize();
	float width = source.m_Width;
	float height = source.m_Height;
	if (m_Stretch == Stretch::Fill) { width = size.m_Width; height = size.m_Height; }
	else if (m_Stretch != Stretch::None)
	{
		const auto ratio = m_Stretch == Stretch::Uniform ? std::min(size.m_Width / width, size.m_Height / height) :
			std::max(size.m_Width / width, size.m_Height / height);
		width *= ratio;
		height *= ratio;
	}
	canvas.DrawImage(m_Image, {(size.m_Width - width) * 0.5f, (size.m_Height - height) * 0.5f, width, height});
}
}
