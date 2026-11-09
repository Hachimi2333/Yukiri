#pragma once

#include <Yukiri/Core/Geometry.hpp>
#include <Yukiri/Text/Font.hpp>
#include <string_view>
#include <cstdint>
#include <vector>
#ifdef DrawText
#undef DrawText
#endif

namespace Yukiri
{
namespace Detail { class DrawList; class TextEngine; class WindowChrome; struct CanvasFactory; }
class Window;
class Image;

class Canvas
{
public:
	void FillRectangle(Rect rectangle, Color color, float radius = 0);
	void StrokeRectangle(Rect rectangle, Color color, float width = 1, float radius = 0);
	void DrawLine(Point start, Point end, Color color, float width = 1);
	void DrawText(std::string_view text, Point position, Color color, float fontSize = 14);
	void DrawImage(const Image& image, Rect rectangle, float opacity = 1);
	[[nodiscard]] Size MeasureText(std::string_view text, float fontSize = 14) const;
	void Save();
	void Restore();
	void SetOffset(Point offset);
	void SetFont(const Font& font);
	void SetOpacity(float opacity);
	[[nodiscard]] float GetOpacity() const noexcept;
	void PushClip(Rect rectangle);
	void PopClip();

private:
	friend class Window;
	friend class Widget;
	friend class Detail::WindowChrome;
	friend struct Detail::CanvasFactory;
	void DrawImage(Rect rectangle, std::uint32_t texture);
	struct State { Point m_Offset; std::vector<Rect> m_Clips; Font m_Font; float m_Opacity = 1; };
	Canvas(Detail::DrawList& list, Size size, Detail::TextEngine* text = nullptr, float scale = 1);
	[[nodiscard]] Rect ToGlobal(Rect rectangle) const;
	[[nodiscard]] Color ApplyOpacity(Color color) const;
	Detail::DrawList& m_List;
	Detail::TextEngine* m_Text;
	float m_Scale;
	State m_State;
	std::vector<State> m_Stack;
};
}
