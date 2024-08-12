#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <memory>
#include <cstdint>
namespace Ball
{
	class CommandQueue;
	namespace GlobalDX12
	{
		// DX12 setup
		extern Microsoft::WRL::ComPtr<ID3D12Device5> g_Device;
		extern Microsoft::WRL::ComPtr<IDXGIFactory4> g_DxgiFactory;
		extern std::shared_ptr<CommandQueue> g_DirectCommandQueue;
		extern Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> g_DirectCommandList;
		extern Microsoft::WRL::ComPtr<ID3D12CommandAllocator> g_CommandAllocator;
		extern Microsoft::WRL::ComPtr<IDXGISwapChain4> g_SwapChain;
		extern uint32_t g_FrameIndex;
		// Useful variables
		extern uint32_t g_RTVDescSize;
		extern uint32_t g_CBV_SRV_UAVDescSize;
		extern uint32_t g_SamplerDescSize;
		// Mip Maps
		extern Microsoft::WRL::ComPtr<ID3D12RootSignature> g_MipMapRootSignature;
		extern Microsoft::WRL::ComPtr<ID3D12PipelineState> g_MipMapPSO;
		extern Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_MipMapHeap;
		extern uint32_t g_MaxMipNumPerTexture; // Assuming the max of 13 mip levels (8k texture)

		extern Microsoft::WRL::ComPtr<ID3D12Resource> g_ScreenshotReadbackBuffer;

		// Query Heap for time stamps
		constexpr uint32_t MAX_GPU_QUERIES = 1000;
		extern uint32_t g_TimestampCounter;
		extern Microsoft::WRL::ComPtr<ID3D12QueryHeap> g_QueryHeap;
		extern Microsoft::WRL::ComPtr<ID3D12Resource> g_ReadbackBuffer;

	} // namespace GlobalDX12
} // namespace Ball