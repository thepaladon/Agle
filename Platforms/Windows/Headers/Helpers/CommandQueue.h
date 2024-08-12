#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <glm/fwd.hpp>
#include <queue>

namespace Ball
{
	class CommandQueue
	{
	public:
		CommandQueue(D3D12_COMMAND_LIST_TYPE type);
		virtual ~CommandQueue();

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> GetCommandList();
		void Flush();
		uint64_t Signal();
		bool IsFenceComplete(uint64_t fenceValue);
		void WaitForFenceValue(uint64_t fenceValue);
		void WaitForExecution(); // TEMPORARY SOLUTION
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;

	protected:
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> CreateCommandList(
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator);

	private:
		// Keep track of command allocators that are "in-flight"
		struct CommandAllocatorEntry
		{
			uint64_t m_FenceValue;
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
		};

		D3D12_COMMAND_LIST_TYPE m_CommandListType;
		Microsoft::WRL::ComPtr<ID3D12Device5> m_D3d12Device;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_D3d12CommandQueue;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_D3d12Fence;
		HANDLE m_FenceEvent;
		uint64_t m_FenceValue;
	};
} // namespace Ball