#include "Rendering/ModelLoading/Model.h"

#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "Rendering/ModelLoading/Material.h"
#include "Rendering/ModelLoading/Mesh.h"

#include <stb/stb_image.h>
#include <TinyglTF/tiny_gltf.h>

#include "Engine.h"
#include "Log.h"
#include "Timer.h"

#include "Rendering/BEAR/BLAS.h"
#include "Rendering/BEAR/Texture.h"
#include "Rendering/ModelLoading/Primitive.h"
#include "Rendering/Renderer.h"
#include "Rendering/ModelLoading/ModelAnimation.h"

#include <mutex>

#include "Rendering/BufferManager.h"

namespace Ball
{
	std::mutex modelMu;

	void Triangle::Draw() const
	{
		const auto& renderer = GetEngine().GetRenderer();
		const auto col = renderer.m_WireframeTriangleLinesColor;
		renderer.DrawDebugLine(m_V0, m_V1, col);
		renderer.DrawDebugLine(m_V1, m_V2, col);
		renderer.DrawDebugLine(m_V2, m_V0, col);
	}

	void Triangle::Draw(glm::mat4& transform) const
	{
		const glm::vec3 v0 = transform * glm::vec4(m_V0, 1.0);
		const glm::vec3 v1 = transform * glm::vec4(m_V1, 1.0);
		const glm::vec3 v2 = transform * glm::vec4(m_V2, 1.0);

		const auto& renderer = GetEngine().GetRenderer();
		const auto col = renderer.m_WireframeTriangleLinesColor;
		renderer.DrawDebugLine(v0, v1, col);
		renderer.DrawDebugLine(v1, v2, col);
		renderer.DrawDebugLine(v2, v0, col);
	}

	struct Node
	{
		glm::mat4 m_Transform = {}; // Transform of Node
		std::vector<int> m_Children; // Idx of nodes which are children to this one
		int32_t m_MeshID = -1; // Idx to Mesh in this node (if there is one)
	};

	struct InBlasConstructor
	{
		std::vector<Node> m_Nodes; // Useful data from tinygltf
		std::vector<Mesh> m_Meshes; // Useful data from tinygltf
		std::vector<Buffer*> m_Buffers; // Pointer to buffer struct stored in Model class
		glm::mat4 m_ModelMatrix; // Transform as we traverse through the nodes
	};

	struct OutBlasConstructor
	{
		std::vector<BLASPrimitive*> m_BlasConstrData; // Data for Creating 1 BLAS
		std::vector<Primitive> m_PrimitiveBufferGPU; // Data needed on GPU to access location of other data
	};

	bool StringEndsWith(const std::string& subject, const std::string& suffix)
	{
		if (suffix.length() > subject.length())
			return false;

		return subject.compare(subject.length() - suffix.length(), suffix.length(), suffix) == 0;
	}

	inline glm::vec3 to_vec3(std::vector<double> array)
	{
		return {static_cast<float>(array[0]), static_cast<float>(array[1]), static_cast<float>(array[2])};
	}

	inline glm::quat to_quat(std::vector<double> array)
	{
		return {static_cast<float>(array[3]),
				static_cast<float>(array[0]),
				static_cast<float>(array[1]),
				static_cast<float>(array[2])};
	}

	// Indices are one after another
	// We group them by 3 manually
	// Careful when passing type of 'indices' because it uses it to traverse through data_ptr
	template<typename T>
	std::vector<uint32_t> ConvertTo32BitIndices(const T* indices, size_t numIndices)
	{
		std::vector<uint32_t> intIndices;
		intIndices.reserve(numIndices);

		for (size_t i = 0; i < numIndices; ++i)
		{
			intIndices.push_back(static_cast<uint32_t>(indices[i]));
		}

		return intIndices;
	}

	// Issues with LoadTextureFromGLTF:
	//  ToDo : This will break for normal maps, they need to be loaded with R8G8B8A8_SNORM
	//  ToDo : Replace with Assert from Logger.h once you update from main
	//  ToDo : This always extends the channels to 4. This is bad. We shouldn't extend it
	Texture* LoadTextureFromGLTF(const tinygltf::Model& model, int index, const std::string& filepath)
	{
		Texture* texture = nullptr;
		TextureSpec spec = {};
		spec.m_Format = TextureFormat::R8G8B8A8_UNORM;
		spec.m_Type = TextureType::R_TEXTURE;
		spec.m_Flags = TextureFlags::MIPMAP_GENERATE;

		int channels;

		std::string name = "Texture [" + std::to_string(index) + "] from " + filepath;

		const auto& image = model.images[index];
		if (image.uri.empty()) // this is in-case of .glb files
		{
			spec.m_Width = image.width;
			spec.m_Height = image.height;

			if (image.bits == 16)
				spec.m_Format = TextureFormat::R16G16B16A16_UNORM;

			// Asserts for some cases we currently don't handle
			ASSERT_MSG(LOG_GRAPHICS,
					   image.component == 4,
					   "Unexpected amount of components (%i) in texture %s.",
					   image.component,
					   image.name.c_str());

			ASSERT_MSG(LOG_GRAPHICS,
					   image.bits != 32,
					   "Unexpected amount of bits (%i) in texture %s.",
					   image.bits,
					   image.name.c_str());

			{
				std::lock_guard<std::mutex> lg(modelMu);
				texture = new Texture(&image.image.at(0), spec, name);
			}
		}
		else // This is in case of .gltf files which store textures as separate .jpgs/.pngs
		{
			std::string uri = model.images[index].uri;
			std::filesystem::path path = std::filesystem::path(filepath).parent_path();
			std::string texturepath = path.string() + "/" + uri;

			int width, height;
			auto* data = stbi_load(texturepath.c_str(), &width, &height, &channels, 4);
			spec.m_Width = width;
			spec.m_Height = height;

			if (data)
			{
				texture = new Texture(data, spec, name);
				stbi_image_free(data);
			}
			else
			{
				// Message: stbi_load unsuccessful
				throw;
			}
		}

		// Message: Texture not loaded successfully
		assert(texture != nullptr);
		return texture;
	}

	// Issues with LoadBufferFromGLTF:
	//  ToDo : The way we currently get vertex data is a "safe" assumption, but still an assumption
	//  ToDo : The way we currently get index buffers always resizes them to uint32_t
	Buffer* LoadBufferFromGLTF(const tinygltf::Model& model, int index, const std::string& filepath)
	{
		const std::string name = "Buffer [" + std::to_string(index) + "] from " + filepath;
		const auto& acc = model.accessors[index];
		const auto& view = model.bufferViews[acc.bufferView];
		const auto& buffer = model.buffers[view.buffer];

		const size_t componentSizeInBytes = tinygltf::GetComponentSizeInBytes(acc.componentType);
		const size_t numElementsInType = tinygltf::GetNumComponentsInType(acc.type);
		void* dataLocation = (void*)&buffer.data.at(view.byteOffset + acc.byteOffset);

		BufferFlags flags = BufferFlags::SRV | BufferFlags::DEFAULT_HEAP;

		// If the buffer is of type float assume Vertex buffer:
		// This is a safe assumption to make, because all "Vertex Buffer" specifies
		// is how he buffer is represented PS5 side.
		if (acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
		{
			flags = flags | BufferFlags::VERTEX_BUFFER;
		}

		// If Index buffer is not uint32_t, resize it to be this way
		if (acc.componentType == TINYGLTF_COMPONENT_TYPE_SHORT ||
			acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
		{
			INFO(LOG_GRAPHICS, "Resizing %s -> uint16_t to uint32_t!", name.c_str());
			const auto u32_from_u16 = ConvertTo32BitIndices((uint16_t*)dataLocation, acc.count);
			dataLocation = (void*)u32_from_u16.data();

			const size_t stride = sizeof(uint32_t) * numElementsInType;
			return BufferManager::CreateBuffer(dataLocation, stride, acc.count, flags, name);
		}

		if (acc.componentType == TINYGLTF_COMPONENT_TYPE_BYTE ||
			acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
		{
			INFO(LOG_GRAPHICS, "Resizing %s -> uint8_t to uint32_t!", name.c_str());
			const auto u32_from_u8 = ConvertTo32BitIndices((uint8_t*)dataLocation, acc.count);
			dataLocation = (void*)u32_from_u8.data();

			const size_t stride = sizeof(uint32_t) * numElementsInType;
			return BufferManager::CreateBuffer(dataLocation, stride, acc.count, flags, name);
		}

		const size_t stride = componentSizeInBytes * numElementsInType;
		return BufferManager::CreateBuffer(dataLocation, stride, acc.count, flags, name);
	}

	void Model::CreateBlasConstructionData(tinygltf::Model& test, OutBlasConstructor& outBlasConstrData,
										   InBlasConstructor inBlasConstrData, const std::vector<int>& evaluatedNode,
										   std::vector<AnimNode>& primOrder)
	{
		// Copy matrix to avoid modifying it as we traverse through.
		const auto parentMatrix = inBlasConstrData.m_ModelMatrix;
		for (const auto nodeIdx : evaluatedNode)
		{
			primOrder[nodeIdx].m_PrimStart = outBlasConstrData.m_PrimitiveBufferGPU.size();
			const auto& node = inBlasConstrData.m_Nodes[nodeIdx];
			const glm::mat4 childMatrix = parentMatrix * node.m_Transform;
			if (node.m_MeshID != -1)
			{
				auto& mesh = inBlasConstrData.m_Meshes[node.m_MeshID];
				for (auto& prim : mesh.GetPrimitivesRef())
				{
					BLASPrimitive* primitiveData = new BLASPrimitive();
					primitiveData->m_ModelMatrix = childMatrix;
					primitiveData->m_IndexBuffer = inBlasConstrData.m_Buffers[prim.GetIndexBufferIndex()];
					primitiveData->m_VertexBuffer = inBlasConstrData.m_Buffers[prim.GetPositionIndex()];
					prim.SetMatrix(childMatrix);

					outBlasConstrData.m_BlasConstrData.push_back(primitiveData);
					outBlasConstrData.m_PrimitiveBufferGPU.push_back(prim);

					if (m_Materials[prim.GetMaterialIndex()].m_Data.m_EmissiveStrength > 1.f)
					{
						PrimitiveLights lights;
						lights.m_NumTriangles =
							inBlasConstrData.m_Buffers[prim.GetIndexBufferIndex()]->GetNumElements() / 3;
						lights.m_PrimitiveID = outBlasConstrData.m_PrimitiveBufferGPU.size() - 1;
						m_Lights.push_back(lights);
					}

					m_CpuPhysicsData.m_PrevMatGpu.push_back(prim.GetMatrix());
				}
			}
			primOrder[nodeIdx].m_PrimEnd = outBlasConstrData.m_PrimitiveBufferGPU.size();
			if (!node.m_Children.empty())
			{
				// Child is parent. Continue traversing down through it's children.
				const auto childNodes = node.m_Children;
				primOrder[nodeIdx].m_ChildNodes = node.m_Children;
				inBlasConstrData.m_ModelMatrix = childMatrix;
				CreateBlasConstructionData(test, outBlasConstrData, inBlasConstrData, childNodes, primOrder);
			}
		}
	}

	Model::Model(const std::string& filepath) : IResourceType(filepath)

	{
		START_TIMER(LoadingModel);

		InBlasConstructor blasHelperData;
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;
		bool res = false;

		tinygltf::Model model;
		// Check which format to load
		if (StringEndsWith(GetPath(), ".gltf"))
			res = loader.LoadASCIIFromFile(&model, &err, &warn, GetPath());
		if (StringEndsWith(GetPath(), ".glb"))
			res = loader.LoadBinaryFromFile(&model, &err, &warn, GetPath());

		if (!warn.empty())
			printf("Warning: %s\n", warn.c_str());

		if (!err.empty())
			printf("Error: %s\n", err.c_str());

		if (!res)
		{
			printf("Failed to load glTF: %s\n", GetPath().c_str());
			assert(res);
		}

		// Load Materials on CPU
		for (int i = 0; i < static_cast<int>(model.materials.size()); i++)
		{
			auto material = Material(model, i);
			m_Materials.push_back(material);
		}

		// Upload Material Buffer to GPU
		{
			std::lock_guard<std::mutex> lg(modelMu);
			m_GPUMaterialBuffer = BufferManager::CreateBuffer(m_Materials.data(),
															  sizeof(Material),
															  m_Materials.size(),
															  BufferFlags::SRV | BufferFlags::DEFAULT_HEAP,
															  "Material Buffer: " + GetPath());
		}

		// Load Buffers (Accessors) to GPU
		for (int i = 0; i < static_cast<int>(model.accessors.size()); i++)
		{
			std::lock_guard<std::mutex> lg(modelMu);
			m_Buffers.push_back(LoadBufferFromGLTF(model, i, GetPath()));
		}

		// Load Meshes and Primitives Indices to CPU
		for (int i = 0; i < static_cast<int>(model.meshes.size()); i++)
		{
			blasHelperData.m_Meshes.emplace_back(model, i);
		}

		// Load Images (texture data)
		for (int i = 0; i < static_cast<int>(model.images.size()); i++)
		{
			m_Textures.push_back(LoadTextureFromGLTF(model, i, GetPath()));
		}

		// Load Animations
		m_Animation = new ModelAnimation();
		m_HasAnimation = m_Animation->LoadAnimations(model);

		// Load Nodes
		// Also handle hierarchy model matrix multiplication
		for (int i = 0; i < static_cast<int>(model.nodes.size()); i++)
		{
			auto position = glm::vec3(0.f);
			auto scale = glm::vec3(1.f);
			auto quat = glm::quat{};
			auto mat = glm::mat4(1.f);
			const auto& node = model.nodes[i];
			auto myNode = Node();

			if (!node.matrix.empty())
			{
				mat = glm::make_mat4(node.matrix.data());
			}
			else
			{
				if (!node.translation.empty())
					position = to_vec3(node.translation);

				if (!node.rotation.empty() && node.rotation.size() == 3)
					quat = glm::quat(to_vec3(node.rotation));

				if (!node.rotation.empty() && node.rotation.size() == 4)
				{
					quat = to_quat(node.rotation);
				}

				if (!node.scale.empty())
					scale = to_vec3(node.scale);

				glm::mat4 matRotation = glm::toMat4(quat);
				glm::mat4 matTransform = glm::translate(glm::mat4(1.0f), position);
				glm::mat4 matScale = glm::scale(glm::mat4(1.0f), scale);

				mat = matTransform * matRotation * matScale;
			}

			myNode.m_Transform = mat;
			myNode.m_MeshID = model.nodes[i].mesh;
			myNode.m_Children = model.nodes[i].children;

			blasHelperData.m_Nodes.push_back(myNode);
		}

		// Nodes and Meshes are already set-up, only the rest is needed
		blasHelperData.m_ModelMatrix = glm::identity<glm::mat4>();
		blasHelperData.m_Buffers = m_Buffers;

		// Create BLAS Construction Data and glTF node based primitive buffer
		m_OutBlasConstrData = new OutBlasConstructor;
		const auto rootNodeIdx = model.scenes[model.defaultScene].nodes;
		m_Animation->GetPrimOrderRef().resize(blasHelperData.m_Nodes.size());

		CreateBlasConstructionData(
			model, *m_OutBlasConstrData, blasHelperData, rootNodeIdx, m_Animation->GetPrimOrderRef());

		BlasQuality blasQuality = m_HasAnimation ? BlasQuality::REFIT_FAST_TRAVERSE : BlasQuality::FAST_TRAVERSE;
		std::string blasName = "BLAS: " + filepath;

		{
			std::lock_guard<std::mutex> lg(modelMu);
			m_BLAS = new BLAS(m_OutBlasConstrData->m_BlasConstrData, blasQuality, blasName);
		}
		// CreateLightTriangleArray(blasHelperData, rootNodeIdx, model, *m_OutBlasConstrData);

		BufferFlags primFlag = m_HasAnimation ? BufferFlags::UPLOAD_HEAP : BufferFlags::DEFAULT_HEAP;
		m_GPUPrimitiveBuffer = BufferManager::CreateBuffer(m_OutBlasConstrData->m_PrimitiveBufferGPU.data(),
														   sizeof(PrimitiveGPU),
														   m_OutBlasConstrData->m_PrimitiveBufferGPU.size(),
														   BufferFlags::SRV | primFlag,
														   "Primitive Buffer: " + GetPath());

		m_CpuPhysicsData.m_PrimitiveBufferGPU = &m_OutBlasConstrData->m_PrimitiveBufferGPU;
		GetCPUTrianglePrimitives(model, blasHelperData.m_Meshes);

		END_TIMER_MSG(LoadingModel, "Finished Loading: %s", GetPath().c_str());
	}

	void Model::RebuildBlas()
	{
		m_BLAS->Update();
		m_GPUPrimitiveBuffer->UpdateData(m_OutBlasConstrData->m_PrimitiveBufferGPU.data(),
										 sizeof(PrimitiveGPU) * m_OutBlasConstrData->m_PrimitiveBufferGPU.size());
	}

	Model::~Model()
	{
	}

	void Model::Unload()
	{
		for (int i = 0; i < m_Buffers.size(); i++)
			BufferManager::DestroyBuffer(m_Buffers[i]);
		for (int i = 0; i < m_Textures.size(); i++)
			delete m_Textures[i];

		BufferManager::DestroyBuffer(m_GPUMaterialBuffer);
		BufferManager::DestroyBuffer(m_GPUPrimitiveBuffer);
		delete m_OutBlasConstrData;
		delete m_Animation;

		// Remove physics trigs and set prims to nullptr
		m_CpuPhysicsData.m_CPUTris.clear();
		m_CpuPhysicsData.m_PrimitiveBufferGPU = nullptr;
	}

	void Model::GetCPUTrianglePrimitives(tinygltf::Model& model_cpu_data, std::vector<Mesh>& meshes)
	{
		for (auto mesh : meshes)
		{
			for (auto primitive : mesh.GetPrimitives())
			{
				std::vector<glm::vec3> positions;
				std::vector<uint32_t> indices;

				// Vertices
				{
					const auto& positionAccessor = model_cpu_data.accessors[primitive.GetPositionIndex()];
					const auto& positionView = model_cpu_data.bufferViews[positionAccessor.bufferView];
					const auto& positionBuffer = model_cpu_data.buffers[positionView.buffer];
					const size_t componentSizeInBytes =
						tinygltf::GetComponentSizeInBytes(positionAccessor.componentType);
					const size_t numElementsInType = tinygltf::GetNumComponentsInType(positionAccessor.type);
					const void* dataLocation =
						(void*)&positionBuffer.data.at(positionView.byteOffset + positionAccessor.byteOffset);
					const size_t dataSize = componentSizeInBytes * numElementsInType * positionAccessor.count;

					positions.resize(positionAccessor.count);
					memcpy(positions.data(), dataLocation, dataSize);
				}

				// Indices
				{
					const auto& indexAccessor = model_cpu_data.accessors[primitive.GetIndexBufferIndex()];
					const auto& indexView = model_cpu_data.bufferViews[indexAccessor.bufferView];
					const auto& indexBuffer = model_cpu_data.buffers[indexView.buffer];
					const void* dataLocation =
						(void*)&indexBuffer.data.at(indexView.byteOffset + indexAccessor.byteOffset);

					// If Index buffer is not uint32_t, resize it to be this way
					if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_SHORT ||
						indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						// INFO(LOG_GRAPHICS, "Resizing %s -> uint16_t to uint32_t!", name.c_str());
						indices = ConvertTo32BitIndices((uint16_t*)dataLocation, indexAccessor.count);
					}
					else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_BYTE ||
							 indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						// INFO(LOG_GRAPHICS, "Resizing %s -> uint8_t to uint32_t!", name.c_str());
						indices = ConvertTo32BitIndices((uint8_t*)dataLocation, indexAccessor.count);
					}
				}

				// Convert all triangles to vector and add them
				std::vector<Triangle> primitiveTriBuffer;
				for (auto i = 0; i < indices.size(); i += 3)
				{
					Triangle triangle;
					triangle.m_V0 = glm::vec4(positions[indices[i + 0]], 1.0);
					triangle.m_V1 = glm::vec4(positions[indices[i + 1]], 1.0);
					triangle.m_V2 = glm::vec4(positions[indices[i + 2]], 1.0);

					primitiveTriBuffer.push_back(triangle);
				}

				auto key = static_cast<uint64_t>(primitive.GetPositionIndex()) << 32 | primitive.GetIndexBufferIndex();

				[[maybe_unused]] auto found = m_CpuPhysicsData.m_CPUTris.find(key);

				assert(found == m_CpuPhysicsData.m_CPUTris.end());

				m_CpuPhysicsData.m_CPUTris.insert({key, primitiveTriBuffer});
			}
		}
	}

	void Model::UpdateAnimations(float curTime)
	{
		if (m_HasAnimation)
			m_Animation->UpdateAnimations(m_OutBlasConstrData, &m_CpuPhysicsData, curTime);
	}

} // namespace Ball