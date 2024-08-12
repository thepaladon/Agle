#pragma once
#include <D3D12/d3dx12.h>
#include "Helpers/RootSignatureGenerator.h"
#include "Helpers/TopLevelASGenerator.h"
#include "Helpers/BottomLevelASGenerator.h"

namespace Ball
{
	struct UserData
	{
		uint64_t m_UserId;
		std::string m_UserName;
	};

	struct BufferData
	{
		D3D12_RESOURCE_STATES m_State = D3D12_RESOURCE_STATE_COMMON;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Buffer = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Uploader = nullptr;
	};

	struct DX12CommandList
	{
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_CommandList = nullptr;
	};

	struct DX12DescriptorHeap
	{
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE m_Handle;
	};

	struct DX12RootSignature
	{
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
		nv_helpers_dx12::RootSignatureGenerator m_Generator;
	};

	struct DX12Sampler
	{
		D3D12_SAMPLER_DESC m_SamplerDesc;
	};

	struct DX12ComputePipeline
	{
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_ComputePSO = nullptr;
	};

	struct DX12RenderTarget
	{
		D3D12_RESOURCE_STATES m_State = D3D12_RESOURCE_STATE_COMMON;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Texture = nullptr;
	};

	struct DX12Texture2D
	{
		D3D12_RESOURCE_STATES m_State = D3D12_RESOURCE_STATE_COMMON;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Texture = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Uploader = nullptr;
	};

	struct DX12Samler
	{
		D3D12_SAMPLER_DESC m_SamplerDesc;
	};

	struct DX12BLAS
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Scratch; // Scratch memory for AS builder
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Result; // Where the AS is
		Microsoft::WRL::ComPtr<ID3D12Resource> m_TransformBuffer; // Hold the matrices of the instances
		nv_helpers_dx12::BottomLevelASGenerator m_BottomLevelASGenerator;
	};

	struct DX12TLAS
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Scratch; // Scratch memory for AS builder
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Result; // Where the AS is
		Microsoft::WRL::ComPtr<ID3D12Resource> m_InstanceDesc; // Hold the matrices of the instances
		nv_helpers_dx12::TopLevelASGenerator m_TopLevelASGenerator;
	};

	typedef DX12RootSignature GPUShaderLayoutHandle;
	typedef BufferData GPUBufferHandle;
	typedef DX12Texture2D GPUTextureHandle;
	typedef DX12RenderTarget GPURenderTarget;
	typedef DX12BLAS GPUBlasHandle;
	typedef DX12TLAS GPUTlasDescHandle;
	typedef DX12DescriptorHeap GPUDescriptorHeapHandle;
	typedef DX12Samler GPUSamplerHandle;
	typedef DX12CommandList GPUCommandListHandle;
	typedef DX12ComputePipeline GPUComputePipelineHandle;
} // namespace Ball
