#include "Window.h"

// Maybe ignore warnings in these include
#ifndef UNICODE
#define UNICODE
#endif

// clang-format off
#include <ImGui/imgui.h>
// clang-format on

#include <Windows.h>
#include <Windowsx.h>

#include "Engine.h"
#include "FileIO.h"
#include "Input/Input.h"
#include "Utilities/LaunchParameters.h"

//----------------------------------------

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Ball
{
	// Used for when going Fullscreen -> Windowed.
	static WINDOWPLACEMENT g_windowBeforeFullscreen = {
		sizeof(WINDOWPLACEMENT), // length of the structure
		0, // flags (no specific flags in this case)
		SW_SHOWNORMAL, // showCmd (normal window)
		{-1, -1}, // ptMinPosition (default minimized position)
		{-1, -1}, // ptMaxPosition (default maximized position)
		// The RECT is set in the Constructor
	};

	Window::Window(uint32_t width, uint32_t height, const std::string& name)
	{
		// Initialize the window class.

		m_WindowData.m_Alive = true;
		m_WindowData.m_Name = name;

		if (LaunchParameters::Contains("Headless"))
			return;

		// Converting from string
		std::wstring stemp = std::wstring(m_WindowData.m_Name.begin(), m_WindowData.m_Name.end());
		LPCWSTR lName = stemp.c_str();

		// Load the icon from a file
		std::string icoPath = FileIO::GetPath(FileIO::Engine, "Images/ballIcon.ico");
		std::wstring wstr = std::wstring(icoPath.begin(), icoPath.end());

		HICON hMyIcon = (HICON)LoadImage(NULL, wstr.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);

		WNDCLASSEX wc = {sizeof(WNDCLASSEX),
						 CS_CLASSDC,
						 WndProc,
						 0L,
						 0L,
						 GetModuleHandle(NULL),
						 hMyIcon,
						 NULL,
						 NULL,
						 NULL,
						 lName,
						 hMyIcon};
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);

		RECT rect;
		auto offset = 50;
		rect.left = offset;
		rect.top = offset;
		rect.right = width + offset;
		rect.bottom = height + offset;

		g_windowBeforeFullscreen.rcNormalPosition = rect;

		// Adjust the window size based on the desired client area size
		AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

#ifdef SHIPPING
		static bool firstFrameFullScreen = true;
#else
		auto fullscreenLP = LaunchParameters::Contains("Fullscreen");
		static bool firstFrameFullScreen = fullscreenLP;
#endif

		auto dwStyle = WS_OVERLAPPEDWINDOW;

		if (firstFrameFullScreen)
		{
			// Assuming you have determined the monitor's dimensions:
			HMONITOR hMonitor = MonitorFromWindow(NULL, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi = {sizeof(MONITORINFO)};
			GetMonitorInfo(hMonitor, &mi);

			dwStyle = WS_POPUP;
			rect.left = mi.rcMonitor.left; // X position (top-left corner of the monitor)
			rect.top = mi.rcMonitor.top; // Y position (top-left corner of the monitor)
			rect.right = mi.rcMonitor.right; // Width (full monitor width)
			rect.bottom = mi.rcMonitor.bottom; // Height (full monitor height)
		}

		RegisterClassEx(&wc);
		HWND hwnd = CreateWindow(wc.lpszClassName,
								 lName,
								 dwStyle,
								 CW_USEDEFAULT,
								 CW_USEDEFAULT,
								 rect.right - rect.left,
								 rect.bottom - rect.top,
								 nullptr,
								 nullptr,
								 wc.hInstance,
								 NULL);

		UpdateWindow(hwnd);
		m_WindowHandle = hwnd;

		RECT finalRenderableRect;
		GetClientRect(hwnd, &finalRenderableRect);
		m_WindowData.m_Width = finalRenderableRect.right - finalRenderableRect.left;
		m_WindowData.m_Height = finalRenderableRect.bottom - finalRenderableRect.top;
	}

	void Window::Init()
	{
		ShowWindow((HWND)m_WindowHandle, SW_SHOWDEFAULT);
	}

	void Window::SetName(const std::string& name)
	{
		m_WindowData.m_Name = name;
		std::wstring stemp = std::wstring(m_WindowData.m_Name.begin(), m_WindowData.m_Name.end());
		LPCWSTR lName = stemp.c_str();
		SetWindowTextW((HWND)m_WindowHandle, lName);
	}

	void Window::Update()
	{
		//  Goes through all messages and events (to process input)
		MSG msg;

		while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		const auto kEnterPrssed = GetInput().GetRawKeyState(KEY_ENTER) == KeyState::PRESSED;
		const auto kAltDown = GetInput().GetRawKeyState(KEY_ALT) == KeyState::DOWN;

		if ((kEnterPrssed && kAltDown))
		{
			ToggleFullscreen();
		}
	}

	bool Window::IsActive() const
	{
		return GetActiveWindow() == static_cast<HWND>(GetWindow().GetWindowHandle());
	}

	void Window::Shutdown()
	{
		m_WindowHandle = nullptr;
	}

	void Window::ToggleFullscreen()
	{
		auto hwnd = (HWND)m_WindowHandle;
		DWORD style = GetWindowLongPtr(hwnd, GWL_STYLE);
		if (style & WS_OVERLAPPEDWINDOW)
		{
			MONITORINFO mi = {sizeof(mi)};
			if (GetWindowPlacement(hwnd, &g_windowBeforeFullscreen) &&
				GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi))
			{
				SetWindowLongPtr(hwnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(hwnd,
							 HWND_TOP,
							 mi.rcMonitor.left,
							 mi.rcMonitor.top,
							 mi.rcMonitor.right - mi.rcMonitor.left,
							 mi.rcMonitor.bottom - mi.rcMonitor.top,
							 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

				m_WindowData.m_Width = GetSystemMetrics(SM_CXSCREEN);
				m_WindowData.m_Height = GetSystemMetrics(SM_CYSCREEN);
				Ball::GetWindow().Resize(m_WindowData.m_Width, m_WindowData.m_Height);
			}
		}
		else
		{
			SetWindowLongPtr(hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
			SetWindowPlacement(hwnd, &g_windowBeforeFullscreen);
			SetWindowPos(
				hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

			RECT rect;
			GetClientRect(hwnd, &rect);

			m_WindowData.m_Width = rect.right - rect.left;
			m_WindowData.m_Height = rect.bottom - rect.top;
			Ball::GetWindow().Resize(m_WindowData.m_Width, m_WindowData.m_Height);
		}
	}
} // namespace Ball

bool g_Maximized = false;
// Dispatches messages or events
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
	//	return true;

	// Switch between msg
	switch (msg)
	{
	case WM_ERASEBKGND:
		{
			return 1;
		}
	case WM_CLOSE:
		{
		}
	case WM_DESTROY:
		{
			ShowCursor(true);
			Ball::GetWindow().Close();

			return 0;
		}
	case WM_EXITSIZEMOVE:
		{
			// Get the updated size.
			RECT r;
			GetClientRect(hWnd, &r);

			uint32_t width = r.right - r.left;
			uint32_t height = r.bottom - r.top;

			Ball::GetWindow().Resize(width, height);
			Ball::GetInput().SetCursorState(Ball::GetInput().GetCursorState());
			break;
		}
	case WM_KILLFOCUS:
		{
			// Remember the state which was before unfocusing the window
			Ball::GetInput().SetPreUnfocusCursorState(Ball::GetInput().GetCursorState());
			// Set the default cursor
			Ball::GetInput().SetCursorState(Ball::CursorState::NONE);
			break;
		}
	case WM_SETFOCUS:
		{
			// Switching tabs causes this to be reset.
			Ball::GetInput().SetCursorState(Ball::GetInput().GetPreUnfocusCursorState());
			break;
		}
	case WM_SIZE:
		{
			Ball::GetInput().SetCursorState(Ball::GetInput().GetCursorState());
			// Check if the window has been maximized
			if (wParam == SIZE_MAXIMIZED && !g_Maximized)
			{
				// Get the updated size.
				RECT r;
				GetClientRect(hWnd, &r);

				uint32_t width = r.right - r.left;
				uint32_t height = r.bottom - r.top;

				Ball::GetWindow().Resize(width, height);

				g_Maximized = true;
			}
			// Check if the window has been restored from maximized state
			else if (wParam == SIZE_RESTORED && g_Maximized)
			{
				// Get the updated size.
				RECT r;
				GetClientRect(hWnd, &r);

				uint32_t width = r.right - r.left;
				uint32_t height = r.bottom - r.top;

				Ball::GetWindow().Resize(width, height);

				g_Maximized = false;
			}
			break;
		}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}
