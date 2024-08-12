#include "Window.h"

#include "Engine.h"
#include "Rendering/Renderer.h"

using namespace Ball;

void Window::Resize(uint32_t width, uint32_t height)
{
	m_WindowData.m_Width = width;
	m_WindowData.m_Height = height;

	GetEngine().OnResize(m_WindowData.m_Width, m_WindowData.m_Height);
}

void Window::SetWidth(uint32_t width)
{
	m_WindowData.m_Width = width;
	GetEngine().OnResize(m_WindowData.m_Width, m_WindowData.m_Height);
};

void Window::SetHeight(uint32_t height)
{
	m_WindowData.m_Height = height;
	GetEngine().OnResize(m_WindowData.m_Width, m_WindowData.m_Height);
}

const std::string& Ball::Window::GetName()
{
	return m_WindowData.m_Name;
}

bool Window::IsAlive() const
{
	return m_WindowData.m_Alive;
}

void Window::Close()
{
	m_WindowData.m_Alive = false;
}
