#include "Rendering/BEAR/Sampler.h"
#include "DX12GlobalVariables.h"
#include <cassert>

namespace Ball
{
	Ball::Sampler::Sampler(MinFilter minFilter, MagFilter magFilter, WrapUV wrapUV)
	{
		D3D12_SAMPLER_DESC samplerDesc = {};
		switch (minFilter)
		{
		case MinFilter::NEAREST:
			if (magFilter == MagFilter::NEAREST)
				samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			else
				samplerDesc.Filter = D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case MinFilter::LINEAR:
			if (magFilter == MagFilter::NEAREST)
				samplerDesc.Filter = D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
			else
				samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case MinFilter::NEAREST_MIPMAP_LINEAR:
			if (magFilter == MagFilter::NEAREST)
				samplerDesc.Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
			else
				samplerDesc.Filter = D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case MinFilter::LINEAR_MIPMAP_NEAREST:
			if (magFilter == MagFilter::NEAREST)
				samplerDesc.Filter = D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
			else
				samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case MinFilter::NEAREST_MIPMAP_NEAREST:
			if (magFilter == MagFilter::NEAREST)
				samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			else
				samplerDesc.Filter = D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case MinFilter::LINEAR_MIPMAP_LINEAR:
			if (magFilter == MagFilter::NEAREST)
				samplerDesc.Filter = D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			else
				samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			break;
		default:
			samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			break;
		}

		switch (wrapUV)
		{
		case WrapUV::REPEAT:
			samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			break;
		case WrapUV::CLAMP_TO_EDGE:
			samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			break;
		case WrapUV::MIRRORED_REPEAT:
			samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			break;
		}
		samplerDesc.MaxLOD = 256;
		m_SamplerHandle.m_SamplerDesc = samplerDesc;
		// Save the sampler enum states
		m_SamplerState = SamplerState(minFilter, magFilter, wrapUV);
	}
} // namespace Ball
