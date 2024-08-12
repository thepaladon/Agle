#pragma once

#include "TypeDefs.h"

namespace Ball
{

	enum class MinFilter
	{
		NEAREST,
		LINEAR,
		NEAREST_MIPMAP_NEAREST,
		LINEAR_MIPMAP_NEAREST,
		NEAREST_MIPMAP_LINEAR,
		LINEAR_MIPMAP_LINEAR
	};

	enum class MagFilter
	{
		NEAREST,
		LINEAR
	};

	enum class WrapUV
	{
		CLAMP_TO_EDGE,
		MIRRORED_REPEAT,
		REPEAT
	};

	struct SamplerState
	{
		SamplerState() { valid = false; }
		SamplerState(MinFilter minFilter, MagFilter magFilter, WrapUV wrapUV)
		{
			MinFilter = minFilter;
			MagFilter = magFilter;
			WrapUV = wrapUV;
			valid = true;
		}

		std::string GetMinFilterName()
		{
			std::string returnString = "null";
			switch (MinFilter)
			{
			case Ball::MinFilter::NEAREST:
				returnString = "nearest";
				break;
			case Ball::MinFilter::LINEAR:
				returnString = "linear";
				break;
			case Ball::MinFilter::NEAREST_MIPMAP_NEAREST:
				returnString = "nearest mipmap nearest";
				break;
			case Ball::MinFilter::LINEAR_MIPMAP_NEAREST:
				returnString = "linear mipmap nearest";
				break;
			case Ball::MinFilter::NEAREST_MIPMAP_LINEAR:
				returnString = "nearest mipmap linear";
				break;
			case Ball::MinFilter::LINEAR_MIPMAP_LINEAR:
				returnString = "linear mipmap linear";
				break;
			}

			return returnString;
		}

		std::string GetMapFilterName()
		{
			std::string returnString = "null";

			switch (MagFilter)
			{
			case Ball::MagFilter::NEAREST:
				returnString = "nearest";
				break;
			case Ball::MagFilter::LINEAR:
				returnString = "linear";
				break;
			}

			return returnString;
		}

		std::string GetWrapUVName()
		{
			std::string returnString = "null";

			switch (WrapUV)
			{
			case Ball::WrapUV::CLAMP_TO_EDGE:
				returnString = "clamp to edge";
				break;
			case Ball::WrapUV::MIRRORED_REPEAT:
				returnString = "mirrored repeat";
				break;
			case Ball::WrapUV::REPEAT:
				returnString = "repeat";
				break;
			}

			return returnString;
		}

		bool valid = false;
		MinFilter MinFilter;
		MagFilter MagFilter;
		WrapUV WrapUV;
	};

	// Define a generic GPU sampler class
	class Sampler
	{
	public:
		// Constructor and destructor
		Sampler(MinFilter minFilter, MagFilter magFilter, WrapUV wrapUV);
		Sampler(const SamplerState& m_SamplerState) :
			Sampler(m_SamplerState.MinFilter, m_SamplerState.MagFilter, m_SamplerState.WrapUV)
		{
		}
		Sampler() = default;
		~Sampler() = default;

		GPUSamplerHandle GetGPUHandle() const { return m_SamplerHandle; }
		GPUSamplerHandle& GetGPUHandleRef() { return m_SamplerHandle; }
		SamplerState GetSamplerState() { return m_SamplerState; }
		SamplerState& GetSamplerStateRef() { return m_SamplerState; }

	private:
		GPUSamplerHandle m_SamplerHandle;
		SamplerState m_SamplerState;
	};

} // namespace Ball
