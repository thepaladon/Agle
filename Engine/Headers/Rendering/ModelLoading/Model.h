#pragma once
#include "ResourceManager/IResourceType.h"
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
	struct OutBlasConstructor;
	struct InBlasConstructor;
	struct AnimNode;
	struct Material;
	class Texture;
	class Mesh;
	class Primitive;
	class Buffer;
	class ModelManager;
	class ResourceDescriptorHeap;
	class BLAS;
	class ModelAnimation;

	struct Triangle
	{
		glm::vec3 m_V0;
		glm::vec3 m_V1;
		glm::vec3 m_V2;
		glm::vec3 m_Normal;

		void Draw() const;
		void Draw(glm::mat4& transform) const;
	};

	struct PrimitiveLights
	{
		uint32_t m_PrimitiveID;
		uint32_t m_NumTriangles;
	};

	struct CpuPhysicsData
	{
		// All Triangles Data for all Meshes of this Model on CPU
		// The Key is a combination of Index Buffer + Vertex Buffer
		// We need it as so, to find the triangles from the `Primitive` vector below
		std::unordered_map<uint64_t, std::vector<Triangle>> m_CPUTris;

		// Ptr to our BLAS Primitive Data which is used to create/update
		// our model BLAS.
		// We iterate through this to know which triangles to render
		// from the data above.
		std::vector<Primitive>* m_PrimitiveBufferGPU;
		std::vector<glm::mat4> m_PrevMatGpu;
	};

	class Model : public IResourceType
	{
		// Required to access Buffers and Textures for adding them to a RDH
		friend ModelManager;

	public:
		Model(const std::string& filepath);
		Model() = delete;
		Model(Model&) = delete;
		Model(Model&&) = delete;
		Model& operator=(const Model&) = delete;
		Model& operator=(Model&&) = delete;

		~Model();

		void Load() override{};
		void Unload() override;

		BLAS& GetBLAS() { return *m_BLAS; }
		void RebuildBlas();

		const std::vector<PrimitiveLights>& GetLightsData() { return m_Lights; }
		int m_ModelIndexID;

		CpuPhysicsData& GetCPUPhysicsData() { return m_CpuPhysicsData; }

		// Animations
		const bool HasAnimation() { return m_HasAnimation; }
		void UpdateAnimations(float curTime);

	private:
		void CreateBlasConstructionData(tinygltf::Model& test, OutBlasConstructor& outBlasConstrData,
										InBlasConstructor inBlasConstrData, const std::vector<int>& evaluatedNode,
										std::vector<AnimNode>& primOrder);

		void GetCPUTrianglePrimitives(tinygltf::Model& model_cpu_data, std::vector<Mesh>& meshes);

		// BLAS Structure on GPU used for TLAS Creation
		BLAS* m_BLAS;

		// Buffer - all buffer specified in the glTF we're loading
		std::vector<Buffer*> m_Buffers;

		// Physics Collision Data
		CpuPhysicsData m_CpuPhysicsData;

		// All textures specified in the glTF were loading
		std::vector<Texture*> m_Textures;

		// Prim Id and Num triangles
		std::vector<PrimitiveLights> m_Lights; // Needed for random sampling

		Buffer* m_GPUMaterialBuffer; // Materials in glTF Specified Order
		Buffer* m_GPUPrimitiveBuffer; // Primitives in Node Order
		std::vector<Material> m_Materials;

		OutBlasConstructor* m_OutBlasConstrData;

		bool m_HasAnimation;
		ModelAnimation* m_Animation = nullptr;
	};

} // namespace Ball