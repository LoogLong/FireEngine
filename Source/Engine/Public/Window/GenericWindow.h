#pragma once
#include <strsafe.h>
#include <tchar.h>
#include <windows.h>
#include <cstdint>
#include <string>

#define WIND_CLASS_NAME _T("Windows Class")


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

namespace DirectX
{
	struct XMMATRIX;
}
namespace FireEngine {
	class CWindowSystem;


	//拆分出去这个文件
	class CWindowsWindow
	{
	public:
		CWindowsWindow() ;
		void Initialize(uint32_t width, uint32_t height, const std::string& title, bool full_screen, CWindowSystem* window_system);
		HWND& GetWindowHwnd() { return  m_hwnd; }

		void PollEvents();

		void GetWindowSize(uint32_t& width, uint32_t& height)
		{
			width = m_width;
			height = m_height;
		}
	private:
		HWND m_hwnd{ nullptr };
		uint32_t m_width{ 1920 };
		uint32_t m_height{ 1080 };
	};
}