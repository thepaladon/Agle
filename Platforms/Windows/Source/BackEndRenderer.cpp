#include "Rendering/BackEndRenderer.h"
#include "Window.h"

#include <dxgidebug.h>
#include "Helpers/TopLevelASGenerator.h"
#include "Helpers/RootSignatureGenerator.h"
#include "Helpers/BottomLevelASGenerator.h"

#include <Helpers/DXHelperFunctions.h>

#include "Utilities/LaunchParameters.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_win32.h>
#include "DX12GlobalVariables.h"

#include <Helpers/CommandQueue.h>
#include <Rendering/BEAR/Texture.h>
#define NUM_RT_BUFFERS 2

#pragma clang diagnostic push

// Disable specific warnings
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#include <ImGui/imgui_impl_dx12.h>
#include <stb/stb_image.h>

#include "Rendering/BufferManager.h"
#include "Rendering/TextureManager.h"

#pragma clang diagnostic pop

// Load DX12 Agility. This is required for devices that don't support Shader model 6.6 and other DX12 Ultimate features
extern "C"
{
	__declspec(dllexport) extern const UINT D3D12SDKVersion = 610;
} // 610 = DX12Agility SDK v1.610.5
extern "C"
{
	__declspec(dllexport) extern const char* D3D12SDKPath = u8".\\DX12Agility\\";
} // Describe where Agility files are located
namespace
{
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_RTVDescHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_ImguiDescHeap;
	// Window
	HWND g_Hwnd;
	bool g_Tearingsupported;
	D3D12_VIEWPORT g_Viewport;
	D3D12_RECT g_ScissorRect;
	Ball::Texture* g_RenderTargets[NUM_RT_BUFFERS];
} // namespace
namespace Ball
{
	BackEndRenderer::BackEndRenderer()
	{
		// Get adapter
		UINT createFactoryFlags = 0;
#if _DEBUG
		if (LaunchParameters::Contains("Debuglayer"))
		{
			createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

			Microsoft::WRL::ComPtr<ID3D12Debug5> m_DebugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_DebugController))))
			{
				m_DebugController->EnableDebugLayer();
				// m_DebugController->SetEnableGPUBasedValidation(true);
			}
		}
#endif

		GlobalDX12::g_DxgiFactory = Helpers::CreateFactory(createFactoryFlags);

		Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter = Helpers::GetAdapter();

		// Create Device
		GlobalDX12::g_Device = Helpers::CreateDevice(adapter);

		// Check hardware sup
		Helpers::CheckRaytracingSupport();

		// Create Commanqueue
		GlobalDX12::g_DirectCommandQueue = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT);

		// Setup utility variables
		GlobalDX12::g_RTVDescSize =
			GlobalDX12::g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		GlobalDX12::g_CBV_SRV_UAVDescSize =
			GlobalDX12::g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		GlobalDX12::g_SamplerDescSize =
			GlobalDX12::g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

		// Setup for Mip Maps
		nv_helpers_dx12::RootSignatureGenerator rsg;
		rsg.AddHeapRangesParameter({CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0)});
		rsg.AddHeapRangesParameter({CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0)});

		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		GlobalDX12::g_MipMapRootSignature = rsg.Generate(GlobalDX12::g_Device.Get());
		psoDesc.pRootSignature = GlobalDX12::g_MipMapRootSignature.Get();

		IDxcBlob* computeShader =
			Helpers::CompileShaderLibrary(FileIO::PlatformSpecificShaders, "CreateMipLevel", L"cs_6_6");
		psoDesc.CS = {computeShader->GetBufferPointer(), computeShader->GetBufferSize()};
		GlobalDX12::g_Device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&GlobalDX12::g_MipMapPSO));

		D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
		uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		uavHeapDesc.NumDescriptors = GlobalDX12::g_MaxMipNumPerTexture;
		uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		GlobalDX12::g_Device->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(&GlobalDX12::g_MipMapHeap));
	}

	void UpdateRenderTargetViews()
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_RTVDescHeap->GetCPUDescriptorHandleForHeapStart());

		for (int i = 0; i < NUM_RT_BUFFERS; ++i)
		{
			ThrowIfFailed(
				GlobalDX12::g_SwapChain->GetBuffer(i, IID_PPV_ARGS(&g_RenderTargets[i]->GetGPUHandleRef().m_Texture)));
			GlobalDX12::g_Device->CreateRenderTargetView(
				g_RenderTargets[i]->GetGPUHandleRef().m_Texture.Get(), nullptr, rtvHandle);
			rtvHandle.ptr += GlobalDX12::g_RTVDescSize;
		}
	}
	void BackEndRenderer::ResizeFrameBuffers(const uint32_t width, const uint32_t height)
	{
		// Don't allow 0 size swap chain back buffers.
		float clientWidth = static_cast<float>(fmax(1u, width));
		float clientHeight = static_cast<float>(fmax(1u, height));
		auto& io = ImGui::GetIO();

		io.DisplaySize = {clientWidth, clientHeight};
		g_Viewport = CD3DX12_VIEWPORT(0.f, 0.f, clientWidth, clientHeight);
		g_ScissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(clientWidth), static_cast<LONG>(clientHeight)),

		// Flush the GPU queue to make sure the swap chain's back buffers
		// are not being referenced by an in-flight command list.
			GlobalDX12::g_DirectCommandQueue->Flush();
		for (int i = 0; i < NUM_RT_BUFFERS; ++i)
		{
			// Any references to the back buffers must be released
			// before the swap chain can be resized.
			g_RenderTargets[i]->Resize(clientWidth, clientHeight);
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(GlobalDX12::g_SwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(GlobalDX12::g_SwapChain->ResizeBuffers(NUM_RT_BUFFERS,
															 static_cast<UINT>(clientWidth),
															 static_cast<UINT>(clientHeight),
															 swapChainDesc.BufferDesc.Format,
															 swapChainDesc.Flags));

		UpdateRenderTargetViews();
	}
	void BackEndRenderer::Initialize(Window* window, Texture** mainRenderTargets, CommandList* cmdList)
	{
		g_Hwnd = static_cast<HWND>(window->GetWindowHandle());
		g_Tearingsupported = false;
		uint32_t width = window->GetWidth();
		uint32_t height = window->GetHeight();
		g_Viewport = CD3DX12_VIEWPORT(0.f, 0.f, static_cast<float>(width), static_cast<float>(height));
		g_ScissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
		//// Create Swapchain
		// TODO format of the swapchain
			GlobalDX12::g_SwapChain = Helpers::CreateSwapChain(GlobalDX12::g_DirectCommandQueue->GetD3D12CommandQueue(),
															   width,
															   height,
															   g_Tearingsupported,
															   g_Hwnd,
															   NUM_RT_BUFFERS);
		////creates rtvs
		g_RTVDescHeap = Helpers::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, NUM_RT_BUFFERS);

		for (int i = 0; i < NUM_RT_BUFFERS; i++)
		{
			g_RenderTargets[i] = mainRenderTargets[i];
		}
		UpdateRenderTargetViews();
		// ImGui Init
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		ImGui::StyleColorsDark();

		// create descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.NumDescriptors = 1;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		ThrowIfFailed(GlobalDX12::g_Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_ImguiDescHeap)));

		ImGui_ImplWin32_Init(g_Hwnd);
		ImGui_ImplDX12_Init(GlobalDX12::g_Device.Get(),
							NUM_RT_BUFFERS,
							DXGI_FORMAT_R8G8B8A8_UNORM,
							g_ImguiDescHeap->GetCPUDescriptorHandleForHeapStart(),
							g_ImguiDescHeap->GetGPUDescriptorHandleForHeapStart());

		// Query Heaps for tracking
		D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
		queryHeapDesc.Count = GlobalDX12::MAX_GPU_QUERIES; // Example size; adjust based on your profiling needs
		queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		queryHeapDesc.NodeMask = 0; // Adjust if using multi-GPU

		GlobalDX12::g_Device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&GlobalDX12::g_QueryHeap));

		// Readback Heap for reading back query Heap data
		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Width = GlobalDX12::MAX_GPU_QUERIES * sizeof(UINT64);
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_READBACK;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 0;
		heapProps.VisibleNodeMask = 0;

		// Create the readback buffer
		HRESULT hr = GlobalDX12::g_Device->CreateCommittedResource(&heapProps,
																   D3D12_HEAP_FLAG_NONE,
																   &bufferDesc,
																   D3D12_RESOURCE_STATE_COPY_DEST,
																   nullptr, // No clear value
																   IID_PPV_ARGS(&GlobalDX12::g_ReadbackBuffer));

		ASSERT_MSG(LOG_GRAPHICS, SUCCEEDED(hr), "Failed the create a readback buffer");
	}

	void BackEndRenderer::EndTracing()
	{
		// Switch to rasterization
		Helpers::TransitionResourceState(g_RenderTargets[GlobalDX12::g_FrameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	void BackEndRenderer::ImguiEndFrame()
	{
#ifndef NO_IMGUI
		// DRAW IMGUI
		ImGui::Render();
		ID3D12DescriptorHeap* ppHeaps[] = {g_ImguiDescHeap.Get()};
		GlobalDX12::g_DirectCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), GlobalDX12::g_DirectCommandList.Get());
#endif
	}
	void BackEndRenderer::EndFrame()
	{
		// TRANSITION RT
		Helpers::TransitionResourceState(g_RenderTargets[GlobalDX12::g_FrameIndex], D3D12_RESOURCE_STATE_PRESENT);
	}

	void BackEndRenderer::BeginFrame()
	{
		// Reset Cmd List
		ThrowIfFailed(GlobalDX12::g_CommandAllocator->Reset());
		ThrowIfFailed(GlobalDX12::g_DirectCommandList->Reset(GlobalDX12::g_CommandAllocator.Get(), nullptr));
		// We RT can be rendered to again
		Helpers::TransitionResourceState(g_RenderTargets[GlobalDX12::g_FrameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);

		// RASTERIZATION
		GlobalDX12::g_DirectCommandList->RSSetViewports(1, &g_Viewport);
		GlobalDX12::g_DirectCommandList->RSSetScissorRects(1, &g_ScissorRect);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_RTVDescHeap->GetCPUDescriptorHandleForHeapStart());

		// start + (index * sizeof a discriptor)
		rtvHandle.ptr = g_RTVDescHeap->GetCPUDescriptorHandleForHeapStart().ptr +
			GlobalDX12::g_FrameIndex * GlobalDX12::g_RTVDescSize;

		GlobalDX12::g_DirectCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		BufferManager::CleanupHelperResources();
		TextureManager::CleanupHelperResources();
	}

	void BackEndRenderer::PresentFrame()
	{
		ThrowIfFailed(GlobalDX12::g_SwapChain->Present(0, 0));
	}
	uint32_t BackEndRenderer::GetCurrentBackBufferIndex() const
	{
		return GlobalDX12::g_FrameIndex;
	}
	static void ReportLiveObjects()
	{
		Microsoft::WRL::ComPtr<IDXGIDebug1> dxgiDebug;
		DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
		dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	}
	void BackEndRenderer::Shutdown()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		GlobalDX12::g_ReadbackBuffer.ReleaseAndGetAddressOf();
		GlobalDX12::g_QueryHeap.ReleaseAndGetAddressOf();
		GlobalDX12::g_CommandAllocator.ReleaseAndGetAddressOf();
		GlobalDX12::g_DirectCommandList.ReleaseAndGetAddressOf();
		GlobalDX12::g_DirectCommandQueue.reset();
		GlobalDX12::g_SwapChain.ReleaseAndGetAddressOf();
		g_RTVDescHeap.ReleaseAndGetAddressOf();
		g_ImguiDescHeap.ReleaseAndGetAddressOf();
		GlobalDX12::g_DxgiFactory.ReleaseAndGetAddressOf();
		GlobalDX12::g_Device.ReleaseAndGetAddressOf();
		atexit(ReportLiveObjects);
	}
	void BackEndRenderer::ImguiBeginFrame()
	{
		// SETUP IMGUI NEW FRAME
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}
	void BackEndRenderer::WaitForCmdQueueExecute()
	{
		////TEMPORARY SOLUTION - LOOK INTO FRAME BUFFERING
		GlobalDX12::g_DirectCommandQueue->WaitForExecution();
	}
} // namespace Ball