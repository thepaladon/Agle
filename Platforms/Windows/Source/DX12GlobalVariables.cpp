#include "DX12GlobalVariables.h"
namespace Ball::GlobalDX12
{
	// DX12 setup
	Microsoft::WRL::ComPtr<ID3D12Device5> g_Device = nullptr;
	Microsoft::WRL::ComPtr<IDXGIFactory4> g_DxgiFactory = nullptr;
	std::shared_ptr<CommandQueue> g_DirectCommandQueue = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> g_DirectCommandList = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> g_CommandAllocator = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> g_SwapChain = nullptr;
	uint32_t g_FrameIndex = 0;
	// Useful variables
	uint32_t g_RTVDescSize = 0;
	uint32_t g_CBV_SRV_UAVDescSize = 0;
	uint32_t g_SamplerDescSize = 0;
	// Mip Maps
	Microsoft::WRL::ComPtr<ID3D12RootSignature> g_MipMapRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> g_MipMapPSO = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_MipMapHeap = nullptr;
	uint32_t g_MaxMipNumPerTexture = 13;

	Microsoft::WRL::ComPtr<ID3D12Resource> g_ScreenshotReadbackBuffer = nullptr;

	uint32_t g_TimestampCounter = 0;
	Microsoft::WRL::ComPtr<ID3D12QueryHeap> g_QueryHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> g_ReadbackBuffer = nullptr;

} // namespace Ball::GlobalDX12