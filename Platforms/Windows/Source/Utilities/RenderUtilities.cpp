#include "Utilities/RenderUtilities.h"
#include "Rendering/BEAR/CommandList.h"

#ifndef SHIPPING
#define USE_PIX
#endif // !SHIPPING
#include <WinPixEventRuntime/Include/WinPixEventRuntime/pix3.h>

#include "DX12GlobalVariables.h"
#include "Log.h"
#include "Helpers/CommandQueue.h"

struct StartEndPairs
{
	std::string name;
	uint32_t start;
	uint32_t end;
};

static std::unordered_map<uint32_t, StartEndPairs> timestampPairs;

namespace Ball::Utilities
{
	void PushGPUMarker(Ball::CommandList* cmdList, const std::string& name)
	{
		MarkerColors color = MarkerColors::TEAL;

		auto it = g_MarkerColors.find(name);
		if (it != g_MarkerColors.end())
			color = it->second;

		PIXBeginEvent(
			cmdList->GetCommandListHandleRef().m_CommandList.Get(), static_cast<uint32_t>(color), name.c_str());
	}

	void SetGPUMarker(Ball::CommandList*, const std::string& name)
	{
		PIXSetMarker(PIX_COLOR(1, 0, 0), name.c_str());
	}

	void PushGPUMarker(Ball::CommandList* cmdList, const std::string& name, MarkerColors color)
	{
		auto it = g_MarkerColors.find(name);
		if (it != g_MarkerColors.end())
			color = it->second;
		else
			color = MarkerColors::TEAL;

		PIXBeginEvent(
			cmdList->GetCommandListHandleRef().m_CommandList.Get(), static_cast<uint32_t>(color), name.c_str());
	}

	void PopGPUMarker(Ball::CommandList* cmdList)
	{
		PIXEndEvent(cmdList->GetCommandListHandleRef().m_CommandList.Get());
	}

#ifndef SHIPPING

	uint32_t PushGPUTimestamp(Ball::CommandList* cmdList, const std::string& name)
	{
		cmdList->GetCommandListHandleRef().m_CommandList.Get()->EndQuery(
			GlobalDX12::g_QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, GlobalDX12::g_TimestampCounter);

		timestampPairs[GlobalDX12::g_TimestampCounter] = StartEndPairs{name, GlobalDX12::g_TimestampCounter, INT32_MAX};

		GlobalDX12::g_TimestampCounter++;

		return GlobalDX12::g_TimestampCounter - 1;
	}

	void PopGPUTimestamp(Ball::CommandList* cmdList, const uint32_t startIndex)
	{
		cmdList->GetCommandListHandleRef().m_CommandList.Get()->EndQuery(
			GlobalDX12::g_QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, GlobalDX12::g_TimestampCounter);

		// Set the end indices to later calculate the time from them.
		auto it = timestampPairs.find(startIndex);
		if (it != timestampPairs.end())
		{
			if (it->second.end == INT32_MAX)
			{
				// If the end value is default (not set), we set an end point
				it->second.end = GlobalDX12::g_TimestampCounter;
			}
			else
			{
				// The the end value is set, we add another entry, the map Index is irrelevant as we always increment at
				// the end
				timestampPairs[GlobalDX12::g_TimestampCounter] =
					StartEndPairs{it->second.name, it->second.start, GlobalDX12::g_TimestampCounter};
			}
		}
		else
		{
			ASSERT_MSG(LOG_GRAPHICS, false, "You're trying to add marker %i that has no start", startIndex);
		}

		GlobalDX12::g_TimestampCounter++;
	}

	void SaveGPUTimestampData(CommandList* cmdList)
	{
		cmdList->GetCommandListHandleRef().m_CommandList.Get()->ResolveQueryData(GlobalDX12::g_QueryHeap.Get(),
																				 D3D12_QUERY_TYPE_TIMESTAMP,
																				 0,
																				 GlobalDX12::g_TimestampCounter,
																				 GlobalDX12::g_ReadbackBuffer.Get(),
																				 0);
	}

	std::vector<TimestampData> ProcessReadbackBuffer()
	{
		std::vector<TimestampData> timestampData;
		timestampData.reserve(timestampPairs.size());

		UINT64* pData = nullptr;
		HRESULT hr = GlobalDX12::g_ReadbackBuffer.Get()->Map(0, nullptr, reinterpret_cast<void**>(&pData));

		UINT64 gpuFrequency;
		GlobalDX12::g_DirectCommandQueue->GetD3D12CommandQueue()->GetTimestampFrequency(&gpuFrequency);

		if (SUCCEEDED(hr))
		{
			for (const auto& ts : timestampPairs)
			{
				UINT64 startTime = pData[ts.second.start];
				UINT64 endTime = pData[ts.second.end];

				const float timeDiffMs = (endTime - startTime) * 1000.0 / gpuFrequency;

				timestampData.push_back({ts.second.name, timeDiffMs});
			}

			GlobalDX12::g_ReadbackBuffer.Get()->Unmap(0, nullptr);
		}
		else
		{
			printf("Testing \n");
			// Handle mapping errors
		}

		// Reset GPU Timestamp Queries
		timestampPairs.clear();
		timestampPairs.reserve(GlobalDX12::MAX_GPU_QUERIES);
		GlobalDX12::g_TimestampCounter = 0;

		return timestampData;
	}
#else
	uint32_t PushGPUTimestamp(CommandList* cmdList, const std::string& name)
	{
		return UINT32_MAX;
	}

	void PopGPUTimestamp(CommandList* cmdList, const uint32_t startIndex)
	{
	}

	void SaveGPUTimestampData(CommandList* cmdList)
	{
	}

	std::vector<TimestampData> ProcessReadbackBuffer()
	{
		return {};
	}

#endif

} // namespace Ball::Utilities