#include "Rendering/BEAR/ComputePipelineDescription.h"

#include "Helpers/DXHelperFunctions.h"
#include "DX12GlobalVariables.h"
#include "Helpers/TempAssert.h"
#include <cassert>

#include "Log.h"
#include "Catch2/catch_amalgamated.hpp"
#include "Utilities/LaunchParameters.h"

namespace Ball
{
	void ComputePipelineDescription::Initialize(const std::string& shaderName, ShaderLayout& layout)
	{
		m_ShaderName = shaderName;
		m_ShaderLayout = layout;
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_ShaderLayout.GetShaderLayoutHandle().m_RootSignature.Get();
		IDxcBlob* computeShader = Helpers::CompileShaderLibrary(FileIO::Shaders, shaderName, L"cs_6_6");
		if (!computeShader || !computeShader->GetBufferPointer())
		{
			// WE assume there was already a error thrown, We don't assert as it would take the programmer out of their
			// flow. its possible they just did a incremental save.
			return;
		}

		psoDesc.CS = {computeShader->GetBufferPointer(), computeShader->GetBufferSize()};
		ThrowIfFailed(
			GlobalDX12::g_Device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineHandle.m_ComputePSO)));

#ifndef SHIPPING
		std::string psoName = shaderName + std::string(" pipeline");
		std::wstring wPsoName = std::wstring(psoName.begin(), psoName.end());
		m_PipelineHandle.m_ComputePSO.Get()->SetName(wPsoName.c_str());
#endif // !SHIPPING

#ifdef _DEBUG
		// Fail save in case this doesn't work with renderdoc ect
		if (!LaunchParameters::Contains("DisableLiveShaders"))
			// Hardcoded file paths, FileWatch is only enabled in debug atm anyway. So lets not try to compile this for
			// release -Damian Bit of a hack which will break if we ever move the build folder, but this should be fine
			// for debug !
			AddFileWatch(FileIO::Shaders, shaderName + ".hlsl");
#endif
	}

	void ComputePipelineDescription::OnFileWatchEvent(const std::string& shader)
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_ShaderLayout.GetShaderLayoutHandle().m_RootSignature.Get();
		IDxcBlob* computeShader = Helpers::CompileShaderLibrary(FileIO::Shaders, m_ShaderName, L"cs_6_6");

		if (!computeShader || !computeShader->GetBufferPointer())
		{
			// WE assume there was already a error thrown, We don't assert as it would take the programmer out of their
			// flow. its possible they just did a incremental save.
			return;
		}
		psoDesc.CS = {computeShader->GetBufferPointer(), computeShader->GetBufferSize()};

		Microsoft::WRL::ComPtr<ID3D12PipelineState> newPSO = nullptr;

		if (FAILED(GlobalDX12::g_Device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&newPSO))))
		{
			ERROR(LOG_GRAPHICS, "Failed to recompile shader: %s", shader.c_str());
			return;
		}

		m_PipelineHandle.m_ComputePSO = newPSO;
	}
} // namespace Ball
