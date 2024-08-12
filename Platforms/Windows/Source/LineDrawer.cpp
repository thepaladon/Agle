#include "Rendering/LineDrawer.h"
#include "D3D12/d3dx12.h"
#include "DX12GlobalVariables.h"
#include "GameObjects/Types/Camera.h"
#include <Helpers/DXHelperFunctions.h>
#include <Helpers/RootSignatureGenerator.h>
#include <dxcapi.h>
#include <glm/ext/matrix_transform.hpp>

#include "Log.h"

namespace Ball
{
	// Rasterization resources for DX12
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_LinePSO;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_LineRootSignature;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_LinesResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_LineVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_LineVertexBufferView;

	void LineDrawer::AddLine(Ball::Line line)
	{
		m_Lines.push_back(line);
	}
	void LineDrawer::Shutdown()
	{
		m_LinePSO.ReleaseAndGetAddressOf();
		m_LineRootSignature.ReleaseAndGetAddressOf();
		m_LinesResource.ReleaseAndGetAddressOf();
		m_LineVertexBuffer.ReleaseAndGetAddressOf();
	}
	void LineDrawer::Init(uint32_t width, uint32_t height)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};

		nv_helpers_dx12::RootSignatureGenerator rsg;
		rsg.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		m_LineRootSignature =
			rsg.Generate(GlobalDX12::g_Device, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		pipelineStateDesc.pRootSignature = m_LineRootSignature.Get();

		Microsoft::WRL::ComPtr<IDxcBlob> lineVS =
			Helpers::CompileShaderLibrary(FileIO::PlatformSpecificShaders, "LineVS", L"vs_6_6");
		Microsoft::WRL::ComPtr<IDxcBlob> linePS =
			Helpers::CompileShaderLibrary(FileIO::PlatformSpecificShaders, "LinePS", L"ps_6_6");
		if (!lineVS || !lineVS->GetBufferPointer() || !linePS || !linePS->GetBufferPointer())
		{
			// WE assume there was already a error thrown, We don't assert as it would take the programmer out of their
			// flow. its possible they just did a incremental save.
			return;
		}

		pipelineStateDesc.VS = {lineVS->GetBufferPointer(), lineVS->GetBufferSize()};
		pipelineStateDesc.PS = {linePS->GetBufferPointer(), linePS->GetBufferSize()};

		pipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		pipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		pipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		pipelineStateDesc.RasterizerState.AntialiasedLineEnable = true;

		pipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		pipelineStateDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		pipelineStateDesc.DepthStencilState.DepthEnable = false;
		pipelineStateDesc.DepthStencilState.StencilEnable = false;

		std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
		pipelineStateDesc.InputLayout = {inputLayout.data(), (UINT)inputLayout.size()};
		pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		pipelineStateDesc.NumRenderTargets = 1;
		pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		pipelineStateDesc.SampleDesc = {1, 0};
		pipelineStateDesc.SampleMask = UINT_MAX;
		GlobalDX12::g_Device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&m_LinePSO));

		// Create a vertex buffer for the line
		glm::vec3 line[2] = {glm::vec3(0), glm::vec3(0.f, -1.f, 0.f)};
		m_LineVertexBuffer = Helpers::CreateBuffer(2 * sizeof(glm::vec3),
												   D3D12_RESOURCE_FLAG_NONE,
												   D3D12_RESOURCE_STATE_GENERIC_READ,
												   Helpers::kUploadHeapProps);
		uint8_t* pData;
		ThrowIfFailed(m_LineVertexBuffer->Map(0, nullptr, (void**)&pData));
		memcpy(pData, &line[0], 2 * sizeof(glm::vec3));
		m_LineVertexBuffer->Unmap(0, nullptr);

		m_LineVertexBufferView.BufferLocation = m_LineVertexBuffer->GetGPUVirtualAddress();
		m_LineVertexBufferView.StrideInBytes = sizeof(glm::vec3);
		m_LineVertexBufferView.SizeInBytes = 2 * sizeof(glm::vec3);

		// Allocate a buffer for line data
		m_LinesResource = Helpers::CreateBuffer(m_MaxLinesNum * sizeof(LineData),
												D3D12_RESOURCE_FLAG_NONE,
												D3D12_RESOURCE_STATE_GENERIC_READ,
												Helpers::kUploadHeapProps);
	}

	void LineDrawer::DrawLines(Camera* cam)
	{
		if (m_Lines.empty())
			return;

		GlobalDX12::g_DirectCommandList->SetPipelineState(m_LinePSO.Get());
		GlobalDX12::g_DirectCommandList->SetGraphicsRootSignature(m_LineRootSignature.Get());

		// Calculations from start->finiswh to MVP
		glm::mat4 viewProjectionMatrix = cam->GetProjection() * cam->GetView();
		std::vector<LineData> lineDatas;
		lineDatas.reserve(m_Lines.size());
		for (auto& line : m_Lines)
		{
			glm::mat4 modelMatrix = glm::translate(glm::identity<glm::mat4>(), line.m_Start);
			glm::vec3 lineDirection = glm::normalize(line.m_End - line.m_Start + glm::vec3(0.0, 0.0, 0.00002));
			float lineLength = glm::length(line.m_End - line.m_Start);
			float angle = glm::acos(abs(glm::dot(lineDirection, glm::vec3(0.0f, -1.0f, 0.0f))));
			if (glm::dot(lineDirection, glm::vec3(0.0f, -1.0f, 0.0f)) < 0.0f)
				angle = glm::pi<float>() - angle;
			modelMatrix = glm::rotate(modelMatrix, angle, glm::cross(glm::vec3(0.0f, -1.0f, 0.0f), lineDirection));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f, lineLength, 1.0f));

			LineData lineData;
			lineData.m_MVP = viewProjectionMatrix * modelMatrix;
			lineData.m_Color = glm::vec4(line.m_Color, 1.0f);
			lineDatas.push_back(lineData);
		}

		ASSERT_MSG(LOG_GRAPHICS,
				   m_Lines.size() <= m_MaxLinesNum,
				   "Exceeded debug line limit of %i. Increase it from LineDrawer.h",
				   m_MaxLinesNum);

		// Copy line data
		uint8_t* pData;
		ThrowIfFailed(m_LinesResource->Map(0, nullptr, (void**)&pData));
		memcpy(pData, lineDatas.data(), lineDatas.size() * sizeof(LineData));
		m_LinesResource->Unmap(0, nullptr);

		// Draw Lines
		GlobalDX12::g_DirectCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		GlobalDX12::g_DirectCommandList->IASetVertexBuffers(0, 1, &m_LineVertexBufferView);
		GlobalDX12::g_DirectCommandList->SetGraphicsRootShaderResourceView(0, m_LinesResource->GetGPUVirtualAddress());
		GlobalDX12::g_DirectCommandList->DrawInstanced(2, lineDatas.size(), 0, 0);
		lineDatas.clear();
		m_Lines.clear();
	}

} // namespace Ball