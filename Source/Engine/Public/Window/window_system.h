#pragma once

#include <memory>
#include <windows.h>

namespace FireEngine
{
	class CWindowsWindow;

	class CWindowSystem : public std::enable_shared_from_this<CWindowSystem>
	{
	public:
		CWindowSystem();
		~CWindowSystem() = default;

		bool WindowShouldClose() const;
		void SetWindowShouldClose(bool should_close)
		{
			m_should_close = should_close;
		}

		void PollEvents();

		HWND GetWindowHwnd();

		std::pair<uint32_t, uint32_t> GetWindowSize();

	private:
		std::unique_ptr<CWindowsWindow> m_windows_window;
		bool m_should_close{ false };
	};
}
