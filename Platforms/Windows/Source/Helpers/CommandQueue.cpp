#include "Headers/Helpers/CommandQueue.h"
#include "DX12GlobalVariables.h"
#include "Helpers/TempAssert.h"
namespace Ball
{
	CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type) :
		m_CommandListType(type), m_D3d12Device(GlobalDX12::g_Device), m_FenceValue(0)
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = type;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		ThrowIfFailed(m_D3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_D3d12CommandQueue)));
		ThrowIfFailed(m_D3d12Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_D3d12Fence)));

		m_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		assert(m_FenceEvent && "Failed to create fence event handle.");
	}

	CommandQueue::~CommandQueue()
	{
	}
	uint64_t CommandQueue::Signal()
	{
		uint64_t fenceValue = ++m_FenceValue;
		m_D3d12CommandQueue->Signal(m_D3d12Fence.Get(), fenceValue);
		return fenceValue;
	}

	bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
	{
		return m_D3d12Fence->GetCompletedValue() >= fenceValue;
	}

	void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
	{
		if (!IsFenceComplete(fenceValue))
		{
			m_D3d12Fence->SetEventOnCompletion(fenceValue, m_FenceEvent);
			::WaitForSingleObject(m_FenceEvent, DWORD_MAX);
		}
	}

	void CommandQueue::Flush()
	{
		WaitForFenceValue(Signal());
	}
	void CommandQueue::WaitForExecution()
	{
		const UINT64 fence = m_FenceValue;
		ThrowIfFailed(m_D3d12CommandQueue->Signal(m_D3d12Fence.Get(), fence));
		m_FenceValue++;

		// Wait until the previous frame is finished.
		if (m_D3d12Fence->GetCompletedValue() < fence)
		{
			ThrowIfFailed(m_D3d12Fence->SetEventOnCompletion(fence, m_FenceEvent));
			WaitForSingleObject(m_FenceEvent, INFINITE);
		}

		GlobalDX12::g_FrameIndex = GlobalDX12::g_SwapChain->GetCurrentBackBufferIndex();
	}

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator()
	{
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
		ThrowIfFailed(m_D3d12Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&commandAllocator)));

		return commandAllocator;
	}

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> CommandQueue::CreateCommandList(
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator)
	{
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList;
		ThrowIfFailed(m_D3d12Device->CreateCommandList(
			0, m_CommandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

		return commandList;
	}

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> CommandQueue::GetCommandList()
	{
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList;
		commandAllocator = CreateCommandAllocator();
		commandList = CreateCommandList(commandAllocator);
		// Associate the command allocator with the command list so that it can be
		// retrieved when the command list is executed.
		ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));
		GlobalDX12::g_CommandAllocator = commandAllocator; // TEMPORARY SOLUTION
		return commandList;
	}
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue::GetD3D12CommandQueue() const
	{
		return m_D3d12CommandQueue;
	}

} // namespace Ball