#include "Headers/Rendering/ModelLoading/ModelAnimation.h"
#include <TinyglTF/tiny_gltf.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "Rendering/BEAR/Buffer.h"
#include <Rendering/ModelLoading/Primitive.h>
#include <Rendering/BEAR/BLAS.h>

namespace Ball
{

	struct OutBlasConstructor
	{
		std::vector<BLASPrimitive*> m_BlasConstrData; // Data for Creating 1 BLAS
		std::vector<Primitive> m_PrimitiveBufferGPU; // Data needed on GPU to access location of other data
	};
	struct Triangle
	{
		glm::vec3 m_V0;
		glm::vec3 m_V1;
		glm::vec3 m_V2;
		glm::vec3 m_Normal;
	};
	struct CpuPhysicsData
	{
		std::unordered_map<uint64_t, std::vector<Triangle>> m_CPUTris;
		std::vector<Primitive>* m_PrimitiveBufferGPU;
		std::vector<glm::mat4> m_PrevMatGpu;
	};

	template<typename T>
	uint32_t ModelAnimation::AddGltfBufferToVector(const tinygltf::Model& model, int acessorId, std::vector<T>& vec,
												   uint32_t& inoutBuffOffset)
	{
		if (m_LoadedBufferOffsets.find(acessorId) == m_LoadedBufferOffsets.end())
		{
			const tinygltf::Accessor& accessor = model.accessors[acessorId];
			auto& view = model.bufferViews[accessor.bufferView];
			const auto& buffer = model.buffers[view.buffer];
			const T* dataLocation = (T*)&buffer.data.at(view.byteOffset + accessor.byteOffset);
			vec.insert(vec.end(), dataLocation, dataLocation + accessor.count);

			m_LoadedBufferOffsets.insert(std::make_pair(acessorId, std::make_pair(inoutBuffOffset, accessor.count)));

			return accessor.count;
		}
		else
		{
			inoutBuffOffset = m_LoadedBufferOffsets[acessorId].first;
			return m_LoadedBufferOffsets[acessorId].second;
		}
		return 0;
	}

	float ModelAnimation::GetInterpolationValue(uint32_t samplerId, float time, uint32_t& frame)
	{
		AnimSampler sampler = m_AnimSamplers[samplerId];
		uint32_t numKeyframes = sampler.m_KeyFrames;
		time = std::fmodf(time, m_TimeKeyFarmes[sampler.m_TimeBuffOffset + numKeyframes - 1]);
		for (uint32_t i = 0; i < numKeyframes - 1; ++i)
		{
			float keyframeTimeStart = m_TimeKeyFarmes[sampler.m_TimeBuffOffset + i];
			float keyframeTimeEnd = m_TimeKeyFarmes[sampler.m_TimeBuffOffset + i + 1];
			if (time >= keyframeTimeStart && time < keyframeTimeEnd)
			{
				// Linear Interp
				float t = (time - keyframeTimeStart) / (keyframeTimeEnd - keyframeTimeStart);
				frame = i;
				return t;
			}
		}
		return 0.f;
	}

	void ModelAnimation::FillInAnimationBuffers(const tinygltf::Model& model, const tinygltf::AnimationSampler& sampler,
												AnimType animType)
	{
		uint32_t offsetB = 0;
		if (animType == ROTATION)
		{
			offsetB = m_RotationKeyFarmes.size();
			AddGltfBufferToVector(model, sampler.output, m_RotationKeyFarmes, offsetB);
		}
		else if (animType == SCALE)
		{
			offsetB = m_ScaleKeyFarmes.size();
			AddGltfBufferToVector(model, sampler.output, m_ScaleKeyFarmes, offsetB);
		}
		else
		{
			offsetB = m_TranslationKeyFarmes.size();
			AddGltfBufferToVector(model, sampler.output, m_TranslationKeyFarmes, offsetB);
		}

		uint32_t offsetT = m_TimeKeyFarmes.size();
		uint32_t numKeyFrames = AddGltfBufferToVector(model, sampler.input, m_TimeKeyFarmes, offsetT);

		Interpolation interp = LINEAR;
		if (sampler.interpolation == "STEP")
			interp = STEP;
		else if (sampler.interpolation == "CUBICSPLINE")
			interp = CUBIC;
		m_AnimSamplers.push_back({offsetT, offsetB, numKeyFrames, interp});
	}

	void ModelAnimation::UpdateNodeAnimationRecursive(OutBlasConstructor* outBlasConstrData, CpuPhysicsData* cpuData,
													  uint32_t nodeId, glm::mat4 prevMat, glm::mat4 newMat)
	{
		// Fill in primitive transforms

		for (uint32_t p = m_RecursivePrimOrder[nodeId].m_PrimStart; p < m_RecursivePrimOrder[nodeId].m_PrimEnd; p++)
		{
			glm::mat4& modelM = outBlasConstrData->m_BlasConstrData[p]->m_ModelMatrix;
			modelM = glm::inverse(prevMat) * modelM;
			modelM = newMat * modelM;

			cpuData->m_PrevMatGpu[p] = outBlasConstrData->m_PrimitiveBufferGPU[p].GetMatrix();
			outBlasConstrData->m_PrimitiveBufferGPU[p].SetMatrix(modelM);
		}
		for (int i = 0; i < m_RecursivePrimOrder[nodeId].m_ChildNodes.size(); i++)
		{
			UpdateNodeAnimationRecursive(
				outBlasConstrData, cpuData, m_RecursivePrimOrder[nodeId].m_ChildNodes[i], prevMat, newMat);
		}
	}

	void ModelAnimation::UpdateNodeAnimation(OutBlasConstructor* outBlasConstrData, CpuPhysicsData* cpuData,
											 AnimChannel& animChanel, float interpVal, uint32_t frame,
											 AnimSampler& sampler, uint32_t nodeId)
	{
		// Calculate the new matrix of the node
		glm::mat4 newMat = outBlasConstrData->m_BlasConstrData[m_RecursivePrimOrder[nodeId].m_PrimStart]->m_ModelMatrix;
		glm::mat4 prevMat = newMat;
		if (animChanel.m_AnimType == TRANSLATION)
		{
			glm::vec3 translChange;
			if (sampler.m_Interpolation == LINEAR)
			{
				translChange = m_TranslationKeyFarmes[sampler.m_AnimBuffOffset + frame] * (1.f - interpVal) +
					m_TranslationKeyFarmes[sampler.m_AnimBuffOffset + frame + 1] * interpVal;
			}
			else if (sampler.m_Interpolation == STEP)
			{
				translChange = m_TranslationKeyFarmes[sampler.m_AnimBuffOffset + frame];
			}

			newMat[3] = glm::vec4(translChange, 1.f);
		}

		if (animChanel.m_AnimType == ROTATION)
		{
			glm::quat rotChange;
			if (sampler.m_Interpolation == LINEAR)
			{
				glm::vec4 prevR = m_RotationKeyFarmes[sampler.m_AnimBuffOffset + frame];
				glm::vec4 nextR = m_RotationKeyFarmes[sampler.m_AnimBuffOffset + frame + 1];
				glm::quat rotThis(prevR.w, prevR.x, prevR.y, prevR.z);
				glm::quat rotNext(nextR.w, nextR.x, nextR.y, nextR.z);
				rotChange = glm::slerp(rotThis, rotNext, interpVal);
			}
			else if (sampler.m_Interpolation == STEP)
			{
				rotChange = glm::quat(m_RotationKeyFarmes[sampler.m_AnimBuffOffset + frame]);
			}
			// Not sure about this order
			rotChange = glm ::normalize(rotChange);

			glm::vec3 scale, translation, skew;
			glm::quat rotat;
			glm::vec4 perspective;
			glm::decompose(newMat, scale, rotat, translation, skew, perspective);

			newMat = glm::scale(glm::translate(glm::mat4(1), translation) * glm::toMat4(rotChange), scale);
		}

		if (animChanel.m_AnimType == SCALE)
		{
			glm::vec3 scaleChange;
			if (sampler.m_Interpolation == LINEAR)
			{
				scaleChange = m_ScaleKeyFarmes[sampler.m_AnimBuffOffset + frame] * (1.f - interpVal) +
					m_ScaleKeyFarmes[sampler.m_AnimBuffOffset + frame + 1] * interpVal;
			}
			else if (sampler.m_Interpolation == STEP)
			{
				scaleChange = m_ScaleKeyFarmes[sampler.m_AnimBuffOffset + frame];
			}
			glm::vec3 scale, translation, skew;
			glm::quat rotat;
			glm::vec4 perspective;
			glm::decompose(newMat, scale, rotat, translation, skew, perspective);

			newMat = glm::scale(glm::translate(glm::mat4(1), translation) * glm::toMat4(rotat), scaleChange);
		}

		// Fill in primitive transforms
		for (uint32_t p = m_RecursivePrimOrder[nodeId].m_PrimStart; p < m_RecursivePrimOrder[nodeId].m_PrimEnd; p++)
		{
			auto& modelM = outBlasConstrData->m_BlasConstrData[p]->m_ModelMatrix;
			modelM = newMat;

			cpuData->m_PrevMatGpu[p] = outBlasConstrData->m_PrimitiveBufferGPU[p].GetMatrix();
			outBlasConstrData->m_PrimitiveBufferGPU[p].SetMatrix(modelM);
		}

		for (int i = 0; i < m_RecursivePrimOrder[nodeId].m_ChildNodes.size(); i++)
		{
			UpdateNodeAnimationRecursive(
				outBlasConstrData, cpuData, m_RecursivePrimOrder[nodeId].m_ChildNodes[i], prevMat, newMat);
		}
	}

	bool ModelAnimation::LoadAnimations(const tinygltf::Model& model)
	{
		bool hasAnimation = false;
		// Load animations
		for (size_t i = 0; i < model.animations.size(); ++i)
		{
			hasAnimation = true;
			const tinygltf::Animation& animation = model.animations[i];
			for (const auto& channel : animation.channels)
			{
				// const tinygltf::Node& node = model.nodes[channel.target_node];
				if (channel.target_path == "translation" || channel.target_path == "rotation" ||
					channel.target_path == "scale") // "Weights - not supported"
				{
					AnimType animType = TRANSLATION;
					if (channel.target_path == "rotation")
						animType = ROTATION;
					else if (channel.target_path == "scale")
						animType = SCALE;

					AnimChannel animChan{uint32_t(m_AnimSamplers.size()), uint32_t(channel.target_node), animType};
					m_AnimChannels.push_back(animChan);
					// This channel represents a transform animation
					const tinygltf::AnimationSampler& sampler = animation.samplers[channel.sampler];
					FillInAnimationBuffers(model, sampler, animType);
				}
			}
		}
		return hasAnimation;
	}

	void ModelAnimation::UpdateAnimations(OutBlasConstructor* outBlasConstrData, CpuPhysicsData* cpuData, float curTime)
	{
		// Load animations
		for (AnimChannel& animChanel : m_AnimChannels)
		{
			uint32_t frame = 0;
			float interpVal = GetInterpolationValue(animChanel.m_Sampler, curTime, frame);

			AnimSampler& sampler = m_AnimSamplers[animChanel.m_Sampler];
			UpdateNodeAnimation(outBlasConstrData, cpuData, animChanel, interpVal, frame, sampler, animChanel.m_Node);
		}
	}
} // namespace Ball