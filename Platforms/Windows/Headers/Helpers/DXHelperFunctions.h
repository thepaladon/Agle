#pragma once

// d3d12 includes
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>

#include <filesystem>
#include <wrl.h>

#include <string>

#include <DirectXMath.h>

#include "FileIO.h"
#include "glm/glm.hpp"
#include "TempAssert.h"

namespace Ball
{
	class Buffer;
	class Texture;
} // namespace Ball
namespace Ball::Helpers
{
	inline std::wstring ConvertToWString(const std::string& str)
	{
		if (str.empty())
			return std::wstring();

		const int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
	}

	static const D3D12_HEAP_PROPERTIES kUploadHeapProps = {
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0,
	};

	static const D3D12_HEAP_PROPERTIES kDefaultHeapProps = {
		D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0};
	enum BufferType
	{
		CBV,
		SRV,
		UAV,
		AS
	};
	DirectX::XMMATRIX ConvertGLMToXMMATRIX(const glm::mat4& glmMatrix);

	void TransitionResourceState(Buffer* buffer, D3D12_RESOURCE_STATES state);
	void TransitionResourceState(Texture* texture, D3D12_RESOURCE_STATES state);
	void TransitionSubResourceState(Texture* texture, D3D12_RESOURCE_STATES stateBefore,
									D3D12_RESOURCE_STATES stateAfter,
									uint32_t subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	void CheckRaytracingSupport();
	void CopyToDirectResource(ID3D12Resource* resource, const void* data, size_t size);
	// Creation functions
	Microsoft::WRL::ComPtr<IDXGIFactory4> CreateFactory(UINT flags);

	Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter();

	Microsoft::WRL::ComPtr<ID3D12Device5> CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter);

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(
		D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_QUEUE_FLAGS flags = D3D12_COMMAND_QUEUE_FLAG_NONE);

	Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain(Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
															uint32_t& width, uint32_t& height, bool& tearingSupported,
															HWND& hwnd, UINT frameCount);

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> CreateDirectCommandList(
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors, bool shaderVisible = false,
		D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	D3D12_CPU_DESCRIPTOR_HANDLE CreateRTV(Microsoft::WRL::ComPtr<ID3D12Resource> pResource,
										  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pHeap, uint32_t& usedHeapEntries,
										  DXGI_FORMAT format);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(uint64_t size, D3D12_RESOURCE_FLAGS flags,
														D3D12_RESOURCE_STATES initState,
														const D3D12_HEAP_PROPERTIES& heapProps);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureBuffer(uint32_t width, uint32_t height, uint32_t mips,
															   DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags,
															   D3D12_RESOURCE_STATES initState,
															   const D3D12_HEAP_PROPERTIES& heapProps);
	IDxcBlob* CompileShaderLibrary(FileIO::DirectoryType location, std::string fileName, LPCWSTR target);
} // namespace Ball::Helpers
