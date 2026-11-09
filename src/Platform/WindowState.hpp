#pragma once

#include <Yukiri/Platform/Window.hpp>
#include <SDL3/SDL.h>
#include "Render/WindowRenderer.hpp"
#include "UI/WidgetTree.hpp"
#include "Text/TextEngine.hpp"
#include "WindowChrome.hpp"
#include "FramePacer.hpp"
#include "FrameStatistics.hpp"
#include "CloseState.hpp"
#include "WindowEvents.hpp"
#include "FileDialogTask.hpp"

namespace Yukiri
{
struct Window::Impl
{
	SDL_Window* m_Native = nullptr;
	WindowConfig m_Config;
	MotionSettings m_Motion;
	Detail::CloseState m_Close;
	Window* m_ParentWindow = nullptr;
	std::vector<std::filesystem::path> m_DroppedFiles;
	Dispatcher m_Dispatcher;
	std::unique_ptr<Detail::FileDialogTask> m_FileDialog;
	bool m_Dirty = true;
	std::unique_ptr<Detail::WindowRenderer> m_Renderer;
	Detail::WidgetTree m_Tree;
	Window* m_Owner = nullptr;
	Detail::TextEngine* m_Text = nullptr;
	std::uint64_t m_BlinkTick = 0;
	Detail::FramePacer m_Pacer;
	Detail::FrameStatistics m_Statistics;
	void RefreshRate();
	[[nodiscard]] Detail::FramePacer::Time NextUpdate(Detail::FramePacer::Time now) const;
	std::unique_ptr<Detail::WindowChrome> m_Chrome;
	std::unique_ptr<Detail::WindowEvents> m_Events;
	std::array<SDL_Cursor*, 5> m_Cursors{};
	void HandleEvent(const SDL_Event& event);
};
}
