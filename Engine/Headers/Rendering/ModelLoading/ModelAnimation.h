#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace tinygltf
{
	class Model;
	struct AnimationSampler;
} // namespace tinygltf

namespace Ball
{
	struct AnimNode
	{
		uint32_t m_PrimStart;
		uint32_t m_PrimEnd;
		std::vector<int> m_ChildNodes;
	};

	// Animations
	enum AnimType
	{
		TRANSLATION = 0,
		ROTATION = 1 << 1,
		SCALE = 1 << 2,
	};

	enum Interpolation
	{
		LINEAR = 0,
		STEP = 1 << 1,
		CUBIC = 1 << 2,
	};

	struct AnimChannel
	{
		uint32_t m_Sampler;
		uint32_t m_Node;
		AnimType m_AnimType;
	};

	struct AnimSampler
	{
		uint32_t m_TimeBuffOffset;
		uint32_t m_AnimBuffOffset;
		uint32_t m_KeyFrames;
		Interpolation m_Interpolation;
	};

	struct OutBlasConstructor;
	struct CpuPhysicsData;

	class ModelAnimation
	{
	public:
		bool LoadAnimations(const tinygltf::Model& model);
		void UpdateAnimations(OutBlasConstructor* outBlasConstrData, CpuPhysicsData* cpuData, float curTime);
		std::vector<AnimNode>& GetPrimOrderRef() { return m_RecursivePrimOrder; }

	private:
		std::vector<AnimChannel> m_AnimChannels;
		std::vector<AnimSampler> m_AnimSamplers;

		std::vector<float> m_TimeKeyFarmes;

		std::vector<glm::vec3> m_TranslationKeyFarmes;
		std::vector<glm::vec4> m_RotationKeyFarmes;
		std::vector<glm::vec3> m_ScaleKeyFarmes;

		std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>
			m_LoadedBufferOffsets; // acessor id, <offset, num elements>

		std::vector<AnimNode>
			m_RecursivePrimOrder; // Each node i of the vector knows its start and end prims in out blas constr
		// Store data related to a gltf animation sampler
		void FillInAnimationBuffers(const tinygltf::Model& model, const tinygltf::AnimationSampler& sampler,
									AnimType animType);
		float GetInterpolationValue(uint32_t samplerId, float time, uint32_t& frame);

		void UpdateNodeAnimation(OutBlasConstructor* outBlasConstrData, CpuPhysicsData* cpuData,
								 AnimChannel& animChanel, float interpVal, uint32_t frame, AnimSampler& sampler,
								 uint32_t nodeId);
		void UpdateNodeAnimationRecursive(OutBlasConstructor* outBlasConstrData, CpuPhysicsData* cpuData,
										  uint32_t nodeId, glm::mat4 prevMat, glm::mat4 newMat);
		template<typename T>
		uint32_t AddGltfBufferToVector(const tinygltf::Model& model, int acessorId, std::vector<T>& vec,
									   uint32_t& inoutBuffOffset);
	};
}; // namespace Ball