#include "Rendering/BEAR/ShaderLayout.h"

#include "DX12GlobalVariables.h"
#include <cassert>

namespace Ball
{

	void ShaderLayout::Initialize()
	{
		m_ShaderLayout.m_RootSignature = m_ShaderLayout.m_Generator.Generate(GlobalDX12::g_Device.Get());
	}

	void ShaderLayout::AddParameter(ShaderParameter type)
	{
		// Register space is always 0 for the sake of cross-platform
		if (type == ShaderParameter::CBV)
		{
			m_ShaderLayout.m_Generator.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, m_NumCBV, 0);
			m_NumCBV++;
		}
		else if (type == ShaderParameter::SRV)
		{
			m_ShaderLayout.m_Generator.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, m_NumSRV, 0);
			m_NumSRV++;
		}
		else
		{
			m_ShaderLayout.m_Generator.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_UAV, m_NumUAV, 0);
			m_NumUAV++;
		}
	}
	void ShaderLayout::Add32bitConstParameter(int num32bit)
	{
		// Register space is always 0 for the sake of cross-platform
		m_ShaderLayout.m_Generator.AddRootParameter(
			D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS, m_NumCBV, 0, D3D12_SHADER_VISIBILITY_ALL, num32bit);
		m_NumCBV++;
	}
	void ShaderLayout::AddParameters(ShaderParameter* type, int num)
	{
		// Register space is always 0 for the sake of cross-platform
		for (int i = 0; i < num; i++)
		{
			if (type[i] == ShaderParameter::CBV)
			{
				m_ShaderLayout.m_Generator.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, m_NumCBV, 0);
				m_NumCBV++;
			}
			else if (type[i] == ShaderParameter::SRV)
			{
				m_ShaderLayout.m_Generator.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, m_NumSRV, 0);
				m_NumSRV++;
			}
			else
			{
				m_ShaderLayout.m_Generator.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_UAV, m_NumUAV, 0);
				m_NumUAV++;
			}
		}
	}
} // namespace Ball
