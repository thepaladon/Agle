#pragma once
#include <string>

namespace Ball
{
	// In a struct since I need to pass this data to wndproc function as a pointer
	struct WindowData
	{
		// Used to trigger things on window events
		// cpp is responsible to set these values when needed
		bool m_Alive = true;

		uint32_t m_Width;
		uint32_t m_Height;
		std::string m_Name;
	};

	class Window
	{
	public:
		Window(uint32_t width, uint32_t height, const std::string& name);
		~Window(){};

		void Close();
		void* GetWindowHandle() { return m_WindowHandle; }
		const uint32_t GetHeight() const { return m_WindowData.m_Height; }
		const uint32_t GetWidth() const { return m_WindowData.m_Width; }
		const std::string& GetName();

		void Resize(uint32_t width, uint32_t height);

		void SetName(const std::string& name);
		void SetWidth(uint32_t width);
		void SetHeight(uint32_t height);
		bool IsAlive() const;
		/// <summary>
		/// Returns true if the Window is currently active.
		/// </summary>
		bool IsActive() const;

	private:
		friend class Engine;
		// Is false when window has been destroyed
		void Init();
		void Update();
		void Shutdown();
		void ToggleFullscreen();

		WindowData m_WindowData = WindowData();
		void* m_WindowHandle;
	};
} // namespace Ball