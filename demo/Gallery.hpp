#pragma once

#include <Yukiri/Yukiri.hpp>

namespace YukiriDemo
{
Yukiri::Result<Yukiri::Window*> OpenGallery(Yukiri::Application& application,
	Yukiri::WindowDecoration decoration = Yukiri::WindowDecoration::Custom,
	Yukiri::ThemeMode theme = Yukiri::ThemeMode::System, bool visible = true);

class Gallery : public Yukiri::ScrollView
{
public:
	Gallery(Yukiri::Application& application, Yukiri::Window& window);
	void Exercise(int frame);
	[[nodiscard]] bool IsThemeSynchronized() const;
private:
	void ApplyTheme(Yukiri::ThemeMode mode);
	void ToggleTheme();
	Yukiri::ComboBox* m_ThemeSelector = nullptr;
	Yukiri::Button* m_ThemeButton = nullptr;
	Yukiri::ToggleSwitch* m_Toggle = nullptr;
	Yukiri::Application& m_Application;
	Yukiri::Window& m_Window;
	Yukiri::Label* m_Status = nullptr;
	Yukiri::TextBox* m_Editor = nullptr;
	Yukiri::Slider* m_Slider = nullptr;
	Yukiri::ProgressBar* m_Progress = nullptr;
	Yukiri::ComboBox* m_Choice = nullptr;
	Yukiri::CheckBox* m_Check = nullptr;
	Yukiri::TabView* m_Tabs = nullptr;
	Yukiri::ScrollView* m_InnerScroll = nullptr;
	int m_ClickCount = 0;
};
}
