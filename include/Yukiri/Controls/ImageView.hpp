#pragma once

#include <Yukiri/Render/Image.hpp>
#include <Yukiri/UI/Widget.hpp>

namespace Yukiri
{
class ImageView : public Widget
{
public:
	enum class Stretch { None, Uniform, UniformToFill, Fill };
	explicit ImageView(Image image = {});
	void SetImage(Image image);
	[[nodiscard]] const Image& GetImage() const noexcept;
	void SetStretch(Stretch stretch);
protected:
	Size OnMeasure(Size available) override;
	void OnPaint(Canvas& canvas) override;
private:
	Image m_Image;
	Stretch m_Stretch = Stretch::Uniform;
};
}
