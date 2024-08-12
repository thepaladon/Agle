#pragma once

#include "Sampler.h"
#include "TypeDefs.h"

namespace Ball
{

	class SamplerDescriptorHeap
	{
	public:
		SamplerDescriptorHeap() = default;
		~SamplerDescriptorHeap() = default;

		void Initialize(uint32_t maxNumberResources, bool fillAllPossible = false);

		// Pushes a sampler to the heap
		int AddSampler(Sampler& sampler);
		// Switches a resource in the heap to the new one
		void SwitchSampler(Sampler& newSampler, uint32_t heapID);

		GPUDescriptorHeapHandle& GetDescriptorHeapHandleRef() { return m_DescriptorHeapHandle; }
		uint32_t GetNumElements() const { return m_NumElements; }

		// Function is inside header because then we dont need two implementations for the two platforms
		// Get string of the texture by id
		SamplerState GetSamplerState(int index)
		{
			SamplerState samplerState = SamplerState();

			// Check if index is valid
			if (index >= 0 && index < m_SamplerStates.size())
			{
				samplerState = m_SamplerStates.at(index);
			}

			return samplerState;
		}

	private:
		GPUDescriptorHeapHandle m_DescriptorHeapHandle;
		uint32_t m_NumElements = 0;
		uint32_t m_MaxSize = 0;
		// Array that stores the samplers with their sampling states for debugging
		std::vector<SamplerState> m_SamplerStates;

		// Fills in all the possible sampler combinations
		void FillDefaultSamplers()
		{
			SamplerState state;
			for (int minF = 0; minF < 6; minF++)
			{
				switch (minF)
				{
				case (0):
					state.MinFilter = Ball::MinFilter::NEAREST;
					break;
				case (1):
					state.MinFilter = Ball::MinFilter::LINEAR;
					break;
				case (2):
					state.MinFilter = Ball::MinFilter::NEAREST_MIPMAP_NEAREST;
					break;
				case (3):
					state.MinFilter = Ball::MinFilter::LINEAR_MIPMAP_NEAREST;
					break;
				case (4):
					state.MinFilter = Ball::MinFilter::NEAREST_MIPMAP_LINEAR;
					break;
				case (5):
					state.MinFilter = Ball::MinFilter::LINEAR_MIPMAP_LINEAR;
					break;
				}
				for (int magF = 0; magF < 2; magF++)
				{
					switch (magF)
					{
					case (0):
						state.MagFilter = Ball::MagFilter::NEAREST;
						break;
					case (1):
						state.MagFilter = Ball::MagFilter::LINEAR;
						break;
					}
					for (int wrapF = 0; wrapF < 3; wrapF++)
					{
						switch (wrapF)
						{
						case (0):
							state.WrapUV = Ball::WrapUV::REPEAT;
							break;
						case (1):
							state.WrapUV = Ball::WrapUV::CLAMP_TO_EDGE;
							break;
						case (2):
							state.WrapUV = Ball::WrapUV::MIRRORED_REPEAT;
							break;
						}
						state.valid = true;
						Sampler sampler = Sampler(state);
						AddSampler(sampler);
					}
				}
			}
		}
	};

} // namespace Ball
