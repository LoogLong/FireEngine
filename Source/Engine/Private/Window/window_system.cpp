

#include "window/window_system.h"

#include "Window/GenericWindow.h"
#include "Core/define.h"


namespace FireEngine {
	CWindowSystem::CWindowSystem()
	{
		m_windows_window = std::make_unique<CWindowsWindow>();
		m_windows_window->Initialize(Config::default_window_size[0], Config::default_window_size[1], "FireEngine", false, this);
	}

	bool CWindowSystem::WindowShouldClose() const
	{
		return m_should_close;
	}

	void CWindowSystem::PollEvents()
	{
		m_windows_window->PollEvents();
	}

	HWND CWindowSystem::GetWindowHwnd()
	{
		return m_windows_window->GetWindowHwnd();
	}

	std::pair<uint32_t, uint32_t> CWindowSystem::GetWindowSize()
	{
		std::pair<uint32_t, uint32_t> result;
		m_windows_window->GetWindowSize(result.first, result.second);
		return result;
	}
}
