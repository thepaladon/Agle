#pragma once
#include <cstdint>

namespace Ball
{
	class Window;
	class Texture;
	class CommandList;
	class BackEndRenderer
	{
	public:
		BackEndRenderer();
		~BackEndRenderer(){};

		void Initialize(Window* window, Texture** mainRenderTargets, CommandList* cmdList);
		void BeginFrame();
		void EndFrame();
		void Shutdown();
		void EndTracing();
		void ImguiBeginFrame();
		void ImguiEndFrame();

		// Resizes Render Targets
		void ResizeFrameBuffers(const uint32_t width, const uint32_t height);

		// Gets the ID of the current Render Target
		uint32_t GetCurrentBackBufferIndex() const;

		void PresentFrame();
		void WaitForCmdQueueExecute();
	};
} // namespace Ball
