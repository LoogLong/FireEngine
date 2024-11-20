
#include <DirectXMath.h>
#include "Window/GenericWindow.h"
#include "window/window_system.h"

namespace FireEngine {

	CWindowSystem* g_window_system = nullptr;

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CLOSE:
			g_window_system->SetWindowShouldClose(true);
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_KEYDOWN:
		{
			const USHORT n16KeyCode = (wParam & 0xFF);
			if (VK_SPACE == n16KeyCode)
			{

			}
			if (VK_ADD == n16KeyCode || VK_OEM_PLUS == n16KeyCode)
			{

			}

			if (VK_SUBTRACT == n16KeyCode || VK_OEM_MINUS == n16KeyCode)
			{

			}

			if ('w' == n16KeyCode || 'W' == n16KeyCode)
			{
			}

			if ('s' == n16KeyCode || 'S' == n16KeyCode)
			{
			}

			if ('d' == n16KeyCode || 'D' == n16KeyCode)
			{
			}

			if ('a' == n16KeyCode || 'A' == n16KeyCode)
			{
			}
			if ('q' == n16KeyCode || 'Q' == n16KeyCode)
			{
			}
			if ('e' == n16KeyCode || 'E' == n16KeyCode)
			{
			}


			if (VK_UP == n16KeyCode)
			{
			}

			if (VK_DOWN == n16KeyCode)
			{
			}

			if (VK_RIGHT == n16KeyCode)
			{
			}

			if (VK_LEFT == n16KeyCode)
			{
			}
		}

		break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}

	FireEngine::CWindowsWindow::CWindowsWindow()
	{
		
	}

	void CWindowsWindow::Initialize(uint32_t width, uint32_t height, const std::string& title, bool full_screen, CWindowSystem* window_system)
	{
		m_width = width;
		m_height = height;
		g_window_system = window_system;
		// 创建窗口
		auto hInstance = GetModuleHandleW(NULL);
		WNDCLASSEX wcex = {};
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_GLOBALCLASS;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH)); //防止无聊的背景重绘
		wcex.lpszClassName = WIND_CLASS_NAME;
		RegisterClassEx(&wcex);

		constexpr DWORD dw_wnd_style = WS_OVERLAPPED | WS_SYSMENU;
		RECT rt_wnd = {0, 0, (LONG)m_width, (LONG)m_height};
		AdjustWindowRect(&rt_wnd, dw_wnd_style, FALSE);

		// 计算窗口居中的屏幕坐标
		const INT pos_x = (GetSystemMetrics(SM_CXSCREEN) - rt_wnd.right - rt_wnd.left) / 2;
		const INT pos_y = (GetSystemMetrics(SM_CYSCREEN) - rt_wnd.bottom - rt_wnd.top) / 2;

		m_hwnd = CreateWindow(WIND_CLASS_NAME
		                      , title.c_str()
		                      , dw_wnd_style
		                      , pos_x
		                      , pos_y
		                      , rt_wnd.right - rt_wnd.left
		                      , rt_wnd.bottom - rt_wnd.top
		                      , nullptr
		                      , nullptr
		                      , hInstance
		                      , nullptr);

		if (!m_hwnd)
		{
			return;
		}

		ShowWindow(m_hwnd, SW_SHOWNA);
		UpdateWindow(m_hwnd);
	}

	void CWindowsWindow::PollEvents()
	{
		//处理消息
		MSG msg = {};
		while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE | PM_NOYIELD))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
}
