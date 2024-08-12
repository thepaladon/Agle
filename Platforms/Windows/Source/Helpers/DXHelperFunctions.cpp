#include "Helpers/DXHelperFunctions.h"
#include "Rendering/BEAR/Buffer.h"
#include "Rendering/BEAR/Texture.h"
#include "DX12GlobalVariables.h"
#include "FileIO.h"
#include "Log.h"

namespace Ball::Helpers
{
	void TransitionResourceState(Buffer* buffer, D3D12_RESOURCE_STATES state)
	{
		if (state == buffer->GetGPUHandleRef().m_State)
			return;
		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = buffer->GetGPUHandleRef().m_Buffer.Get();
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = buffer->GetGPUHandleRef().m_State;
		barrierDesc.Transition.StateAfter = state;
		GlobalDX12::g_DirectCommandList->ResourceBarrier(1, &barrierDesc);
		buffer->GetGPUHandleRef().m_State = state;
	}

	void TransitionResourceState(Texture* texture, D3D12_RESOURCE_STATES state)
	{
		if (state == texture->GetGPUHandleRef().m_State)
			return;
		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = texture->GetGPUHandleRef().m_Texture.Get();
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = texture->GetGPUHandleRef().m_State;
		barrierDesc.Transition.StateAfter = state;
		GlobalDX12::g_DirectCommandList->ResourceBarrier(1, &barrierDesc);
		texture->GetGPUHandleRef().m_State = state;
	}

	void TransitionSubResourceState(Texture* texture, D3D12_RESOURCE_STATES stateBefore,
									D3D12_RESOURCE_STATES stateAfter, uint32_t subresource)
	{
		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = texture->GetGPUHandleRef().m_Texture.Get();
		barrierDesc.Transition.Subresource = subresource;
		barrierDesc.Transition.StateBefore = stateBefore;
		barrierDesc.Transition.StateAfter = stateAfter;
		GlobalDX12::g_DirectCommandList->ResourceBarrier(1, &barrierDesc);
	}

	void CopyToDirectResource(ID3D12Resource* resource, const void* data, size_t size)
	{
		CD3DX12_RESOURCE_BARRIER transition =
			CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		GlobalDX12::g_DirectCommandList->ResourceBarrier(1, &transition);

		Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
		CD3DX12_RANGE readRange(0, 0);
		uploadBuffer =
			CreateBuffer(size, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
		UINT8* pUploadBegin;
		ThrowIfFailed(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pUploadBegin)));
		memcpy(pUploadBegin, data, size);
		uploadBuffer->Unmap(0, nullptr);
		GlobalDX12::g_DirectCommandList->CopyResource(resource, uploadBuffer.Get());
		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
		GlobalDX12::g_DirectCommandList->ResourceBarrier(1, &transition);
	}
	void CheckRaytracingSupport()
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
		ThrowIfFailed(
			GlobalDX12::g_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)));

		if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
		{
			MessageBox(NULL, L"Your current graphics card does not support raytracing", L"Hardware unsupported", MB_OK);
			throw std::runtime_error("Raytracing not supported on device");
		}
	}
	DirectX::XMMATRIX ConvertGLMToXMMATRIX(const glm::mat4& glmMatrix)
	{
		DirectX::XMMATRIX xmMatrix;

		// Copy elements, taking into account memory layout differences
		for (int col = 0; col < 4; ++col)
		{
			for (int row = 0; row < 4; ++row)
			{
				xmMatrix.r[col][row] = glmMatrix[col][row];
			}
		}

		return xmMatrix;
	}
	// Creation functions
	Microsoft::WRL::ComPtr<IDXGIFactory4> CreateFactory(UINT flags)
	{
		Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory = {};
		ThrowIfFailed(CreateDXGIFactory2(flags, IID_PPV_ARGS(&dxgiFactory)));
		return dxgiFactory;
	}

	Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter()
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiAdapter1 = {};
		Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter4 = {};

		SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; GlobalDX12::g_DxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			// Check to see if the adapter can create a D3D12 device without actually
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(
					D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}
		}

		return dxgiAdapter4;
	}

	Microsoft::WRL::ComPtr<ID3D12Device5> CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter)
	{
		Microsoft::WRL::ComPtr<ID3D12Device5> d3d12Device5;
		ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device5)));

		// Enable debug messages in debug mode.
#if defined(_DEBUG)
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> pInfoQueue;

		if (SUCCEEDED(d3d12Device5.As(&pInfoQueue)))
		{
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

			// Suppress messages based on their severity level
			D3D12_MESSAGE_SEVERITY Severities[] = {D3D12_MESSAGE_SEVERITY_INFO};

			D3D12_INFO_QUEUE_FILTER NewFilter = {};

			NewFilter.DenyList.NumSeverities = _countof(Severities);
			NewFilter.DenyList.pSeverityList = Severities;

			ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
		}
#endif

		return d3d12Device5;
	}

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type,
																  D3D12_COMMAND_QUEUE_FLAGS flags)
	{
		// Describe and create the command queue.
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandqueue;
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = flags;
		queueDesc.Type = type;

		ThrowIfFailed(GlobalDX12::g_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandqueue)));
		return commandqueue;
	}

	Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain(Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
															uint32_t& width, uint32_t& height, bool& tearingSupported,
															HWND& hwnd, UINT frameCount)
	{
		Microsoft::WRL::ComPtr<IDXGISwapChain4> dxgiSwapChain4;

		BOOL allowTearing = FALSE;

		// Rather than create the DXGI 1.5 factory interface directly, we create the
		// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the
		// graphics debugging tools which will not support the 1.5 factory interface
		// until a future update.
		Microsoft::WRL::ComPtr<IDXGIFactory4> factory4;
		if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
		{
			Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
			if (SUCCEEDED(factory4.As(&factory5)))
			{
				if (FAILED(factory5->CheckFeatureSupport(
						DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
				{
					allowTearing = FALSE;
				}
			}
		}

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = width;
		swapChainDesc.Height = height;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.Stereo = FALSE;
		swapChainDesc.SampleDesc = {1, 0};
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = frameCount;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		// It is recommended to always allow tearing if tearing support is available.
		swapChainDesc.Flags = allowTearing == TRUE ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
		swapChainDesc.SampleDesc.Count = 1;
		tearingSupported = allowTearing;

		Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
		ThrowIfFailed(GlobalDX12::g_DxgiFactory->CreateSwapChainForHwnd(
			commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));

		// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
		// will be handled manually.
		ThrowIfFailed(GlobalDX12::g_DxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

		ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

		return dxgiSwapChain4;
	}

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> CreateDirectCommandList(
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator)
	{
		// Create the command list.
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandlist;
		ThrowIfFailed(GlobalDX12::g_Device->CreateCommandList(
			0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&commandlist)));
		return commandlist;
	}

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type,
																	  uint32_t numDescriptors, bool shaderVisible,
																	  D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	{
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = numDescriptors;
		desc.Type = type;
		desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : flags;
		ThrowIfFailed(GlobalDX12::g_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

		return descriptorHeap;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE CreateRTV(Microsoft::WRL::ComPtr<ID3D12Resource> pResource,
										  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pHeap, uint32_t& usedHeapEntries,
										  DXGI_FORMAT format)
	{
		D3D12_RENDER_TARGET_VIEW_DESC desc = {};
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Format = format;
		desc.Texture2D.MipSlice = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr +=
			usedHeapEntries * GlobalDX12::g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		usedHeapEntries++;

		// Create the RTV without modifying the original rtvHandle
		GlobalDX12::g_Device->CreateRenderTargetView(pResource.Get(), &desc, rtvHandle);

		// Return the original rtvHandle
		return rtvHandle;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(uint64_t size, D3D12_RESOURCE_FLAGS flags,
														D3D12_RESOURCE_STATES initState,
														const D3D12_HEAP_PROPERTIES& heapProps)
	{
		D3D12_RESOURCE_DESC bufDesc = {};
		bufDesc.Alignment = 0;
		bufDesc.DepthOrArraySize = 1;
		bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufDesc.Flags = flags;
		bufDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufDesc.Height = 1;
		bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufDesc.MipLevels = 1;
		bufDesc.SampleDesc.Count = 1;
		bufDesc.SampleDesc.Quality = 0;
		bufDesc.Width = size;

		Microsoft::WRL::ComPtr<ID3D12Resource> pBuffer;
		ThrowIfFailed(GlobalDX12::g_Device->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, initState, nullptr, IID_PPV_ARGS(&pBuffer)));
		return pBuffer;
	}
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureBuffer(uint32_t width, uint32_t height, uint32_t mips,
															   DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags,
															   D3D12_RESOURCE_STATES initState,
															   const D3D12_HEAP_PROPERTIES& heapProps)
	{
		D3D12_RESOURCE_DESC bufDesc = {};
		bufDesc.Alignment = 0;
		bufDesc.DepthOrArraySize = 1;
		bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		bufDesc.Flags = flags;
		bufDesc.Format = format;
		bufDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		bufDesc.MipLevels = mips;
		bufDesc.SampleDesc.Count = 1;
		bufDesc.SampleDesc.Quality = 0;
		bufDesc.Width = width;
		bufDesc.Height = height;

		Microsoft::WRL::ComPtr<ID3D12Resource> pBuffer;
		ThrowIfFailed(GlobalDX12::g_Device->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, initState, nullptr, IID_PPV_ARGS(&pBuffer)));
		return pBuffer;
	}
	//--------------------------------------------------------------------------------------------------
	// Compile a HLSL file into a DXIL library
	//

	IDxcBlob* CompileShaderLibrary(FileIO::DirectoryType location, std::string fileName, LPCWSTR target)
	{
		// Consider moving Shaders/ out of here, as it is more explicit and will be better for cross platform
		Microsoft::WRL::ComPtr<IDxcCompiler> pCompiler = nullptr;
		Microsoft::WRL::ComPtr<IDxcLibrary> pLibrary = nullptr;
		Microsoft::WRL::ComPtr<IDxcIncludeHandler> dxcIncludeHandler;

		HRESULT hr;

		// Initialize the DXC compiler and compiler helper
		if (!pCompiler)
		{
			if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (void**)&pCompiler)))
			{
				ERROR(LOG_GRAPHICS, "DxcCreateInstance for compiler failed!");
				return nullptr;
			}

			if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void**)&pLibrary)))
			{
				ERROR(LOG_GRAPHICS, "DxcCreateInstance for libary failed!");
				return nullptr;
			}

			if (FAILED(pLibrary->CreateIncludeHandler(&dxcIncludeHandler)))
			{
				ERROR(LOG_GRAPHICS, "Create IncludeHandlers failed!");
				return nullptr;
			}
		}

		std::string sShader = FileIO::Read(location, fileName + ".hlsl");

		if (sShader.empty())
		{
			ERROR(LOG_GRAPHICS, "Read shader file is empty ");
			return nullptr;
		}

		// Create blob from the string
		Microsoft::WRL::ComPtr<IDxcBlobEncoding> pTextBlob;

		if (FAILED(pLibrary->CreateBlobWithEncodingFromPinned(
				(LPBYTE)sShader.c_str(), (uint32_t)sShader.size(), 0, &pTextBlob)))
		{
			ERROR(LOG_GRAPHICS, "CreateBlobWithEncodingFromPinned failed!");
			return nullptr;
		}

		auto fullFilePath = FileIO::GetPath(location, fileName + ".hlsl");
		std::wstring wFileName(fullFilePath.begin(), fullFilePath.end());

		std::vector<LPCWSTR> arguments;
#ifndef SHIPPING
		arguments.push_back(L"-Zi");
#endif // !SHIPPING

		// Compile
		Microsoft::WRL::ComPtr<IDxcOperationResult> m_Result;
		if (FAILED(pCompiler->Compile(pTextBlob.Get(),
									  wFileName.c_str(),
									  L"main",
									  target,
									  arguments.data(),
									  static_cast<UINT32>(arguments.size()),
									  nullptr,
									  0,
									  dxcIncludeHandler.Get(),
									  &m_Result)))
		{
			ERROR(LOG_GRAPHICS, "Compile failed!");
			return nullptr;
		}

		// Verify the result
		HRESULT resultCode;
		if (FAILED(m_Result->GetStatus(&resultCode)))
		{
			ERROR(LOG_GRAPHICS, "Failed to get result status !");
			return nullptr;
		}

		if (FAILED(resultCode))
		{
			Microsoft::WRL::ComPtr<IDxcBlobEncoding> pError;
			hr = m_Result->GetErrorBuffer(&pError);
			if (FAILED(hr))
			{
				ERROR(LOG_GRAPHICS, "Failed to get shader compiler error");
				return nullptr;
			}

			// Convert error blob to a string
			std::vector<char> infoLog(pError->GetBufferSize() + 1);
			memcpy(infoLog.data(), pError->GetBufferPointer(), pError->GetBufferSize());
			infoLog[pError->GetBufferSize()] = 0;

			ERROR(LOG_GRAPHICS, "Failed to compile shader: '%s'", infoLog.data());
		}

		IDxcBlob* pBlob;
		if (FAILED(m_Result->GetResult(&pBlob)))
		{
			ERROR(LOG_GRAPHICS, "Failed to get result shader blob");
			return nullptr;
		}

		return pBlob;
	}
} // namespace Ball::Helpers