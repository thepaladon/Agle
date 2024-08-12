#include "Rendering/ModelLoading/Material.h"
#include "Rendering/ModelLoading/GltfExtentionReader.h"
#include <TinyglTF/tiny_gltf.h>
#include <string>

namespace Ball
{

	MaterialGPU GetDefaultMaterial()
	{
		MaterialGPU defaultMaterial;
		defaultMaterial.m_BaseColorTextureIndex = -1;
		defaultMaterial.m_MetallicRoughnessTextureIndex = -1;
		defaultMaterial.m_EmissiveTextureIndex = -1;
		defaultMaterial.m_NormalTextureIndex = -1;
		defaultMaterial.m_BaseColorSamplerIndex = -1;
		defaultMaterial.m_MetallicRoughnessSamplerIndex = -1;
		defaultMaterial.m_EmissiveSamplerIndex = -1;
		defaultMaterial.m_NormalSamplerIndex = -1;

		defaultMaterial.m_BaseColorFactor = {1.f, 1.f, 1.f, 1.f};
		defaultMaterial.m_MetallicFactor = 1.f;
		defaultMaterial.m_RoughnessFactor = 1.f;
		defaultMaterial.m_EmissiveFactor = {1.f, 1.f, 1.f};
		defaultMaterial.m_NormalTextureScale = 1.f;

		// KHR_materials_specular
		defaultMaterial.m_SpecularFactor = 0.f;
		defaultMaterial.m_SpecularTextureIndex = -1;
		defaultMaterial.m_SpecularSamplerIndex = -1;
		defaultMaterial.m_SpecularColorFactor = {1.f, 1.f, 1.f};
		defaultMaterial.m_SpecularColorTextureIndex = -1;
		defaultMaterial.m_SpecularColorSamplerIndex = -1;

		// KHR_materials_transmission
		defaultMaterial.m_TransmissionFactor = 0.f;
		defaultMaterial.m_TransmissionTextureIndex = -1;
		defaultMaterial.m_TransmissionSamplerIndex = -1;

		// KHR_materials_unlit
		defaultMaterial.m_Unlit = 0.f;

		// KHR_materials_ior
		defaultMaterial.m_Ior = 1.5f;

		// KHR_materials_volume
		defaultMaterial.m_AttenuationDistance = std::numeric_limits<float>::max();
		defaultMaterial.m_AttenuationColor = {1.f, 1.f, 1.f};

		// KHR_materials_emissive_strength
		defaultMaterial.m_EmissiveStrength = 1.f;
		// Dimension for the Mip-Map generation
		defaultMaterial.m_TextureDim = 1.f; // 1.f, as log2(1.f) = 0.f

		return defaultMaterial;
	};
	uint32_t GetSamplerHeapIndexFromGLTF(tinygltf::Sampler sampler)
	{
		// setup filters for the sampler based on the samplers m_From the GLTF model
		uint32_t minMultiplier = 0;
		uint32_t magMultiplier = 0;
		uint32_t UVMultiplier = 0;
		switch (sampler.minFilter)
		{
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
			minMultiplier = 0;
			break;
		case TINYGLTF_TEXTURE_FILTER_LINEAR:
			minMultiplier = 1;
			break;
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
			minMultiplier = 2;
			break;
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
			minMultiplier = 3;
			break;
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
			minMultiplier = 4;
			break;
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
			minMultiplier = 5;
			break;
		default:
			minMultiplier = 0;
			break;
		}
		if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
			magMultiplier = 0;
		else
			magMultiplier = 1;

		switch (sampler.wrapS)
		{
		case TINYGLTF_TEXTURE_WRAP_REPEAT:
			UVMultiplier = 0;
			break;
		case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
			UVMultiplier = 1;
			break;
		case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
			UVMultiplier = 2;
			break;
		}

		return minMultiplier * 6 + magMultiplier * 3 + UVMultiplier; // Mimicking the way we've put them in the heap
	}

	Material::Material(const tinygltf::Model& model, int index)
	{
		m_Data = GetDefaultMaterial();

		auto& mat = model.materials[index];
		auto& textures = model.textures;

		// Tiny gltf reading, inspired by https://github.com/nvpro-samples/nvpro_core/blob/master/nvh/gltfscene.cpp
		// Base gltf PBR values
		auto& pbrMR = mat.pbrMetallicRoughness;
		m_Data.m_BaseColorFactor = glm::vec4(
			pbrMR.baseColorFactor[0], pbrMR.baseColorFactor[1], pbrMR.baseColorFactor[2], pbrMR.baseColorFactor[3]);
		if (pbrMR.baseColorTexture.index >= 0)
		{
			m_Data.m_BaseColorTextureIndex = model.textures[pbrMR.baseColorTexture.index].source;
			uint32_t samplerIndex = model.textures[pbrMR.baseColorTexture.index].sampler;
			m_Data.m_BaseColorSamplerIndex = GetSamplerHeapIndexFromGLTF(model.samplers[samplerIndex]);
			m_Data.m_TextureDim = (float)model.images[m_Data.m_BaseColorTextureIndex].width;
		}

		m_Data.m_MetallicFactor = static_cast<float>(pbrMR.metallicFactor);
		m_Data.m_RoughnessFactor = static_cast<float>(pbrMR.roughnessFactor);
		if (pbrMR.metallicRoughnessTexture.index >= 0)
		{
			m_Data.m_MetallicRoughnessTextureIndex = model.textures[pbrMR.metallicRoughnessTexture.index].source;
			uint32_t samplerIndex = model.textures[pbrMR.metallicRoughnessTexture.index].sampler;
			m_Data.m_MetallicRoughnessSamplerIndex = GetSamplerHeapIndexFromGLTF(model.samplers[samplerIndex]);
			if (m_Data.m_TextureDim == 1.f)
				m_Data.m_TextureDim = (float)model.images[m_Data.m_MetallicRoughnessTextureIndex].width;
		}

		m_Data.m_EmissiveFactor = mat.emissiveFactor.size() == 3
			? glm::vec3(mat.emissiveFactor[0], mat.emissiveFactor[1], mat.emissiveFactor[2])
			: glm::vec3(0.f);
		if (mat.emissiveTexture.index >= 0)
		{
			m_Data.m_EmissiveTextureIndex = model.textures[mat.emissiveTexture.index].source;
			uint32_t samplerIndex = model.textures[mat.emissiveTexture.index].sampler;
			m_Data.m_EmissiveSamplerIndex = GetSamplerHeapIndexFromGLTF(model.samplers[samplerIndex]);
			if (m_Data.m_TextureDim == 1.f)
				m_Data.m_TextureDim = (float)model.images[m_Data.m_EmissiveTextureIndex].width;
		}

		m_Data.m_NormalTextureScale = static_cast<float>(mat.normalTexture.scale);
		if (mat.normalTexture.index >= 0)
		{
			m_Data.m_NormalTextureIndex = model.textures[mat.normalTexture.index].source;
			uint32_t samplerIndex = model.textures[mat.normalTexture.index].sampler;
			m_Data.m_NormalSamplerIndex = GetSamplerHeapIndexFromGLTF(model.samplers[samplerIndex]);
			if (m_Data.m_TextureDim == 1.f)
				m_Data.m_TextureDim = (float)model.images[m_Data.m_NormalTextureIndex].width;
		}

		auto extentionIter = mat.extensions.find("KHR_materials_specular");
		if (extentionIter != mat.extensions.end())
		{
			// From the GLTF spec the default m_SpecularFactor = 1.f, however, blender exports it the wrong way
			// So, the default for their renderer is 0.5f, but if you do not change it, blender exports 0.f, so we have
			// to adjust to it
			const auto& extVals = extentionIter->second;
			GetFloat(extVals, "specularFactor", m_Data.m_SpecularFactor);
			GetTexAndSampId(
				extVals, textures, "specularTexture", m_Data.m_SpecularTextureIndex, m_Data.m_SpecularSamplerIndex);
			GetVec3(extVals, "specularColorFactor", m_Data.m_SpecularColorFactor);
			GetTexAndSampId(extVals,
							textures,
							"specularColorTexture",
							m_Data.m_SpecularColorTextureIndex,
							m_Data.m_SpecularColorSamplerIndex);
		}

		// KHR_materials_unlit
		extentionIter = mat.extensions.find("KHR_materials_unlit");
		if (extentionIter != mat.extensions.end())
		{
			m_Data.m_Unlit = 1;
		}

		// KHR_materials_transmission
		extentionIter = mat.extensions.find("KHR_materials_transmission");
		if (extentionIter != mat.extensions.end())
		{
			const auto& extVals = extentionIter->second;
			GetFloat(extVals, "transmissionFactor", m_Data.m_TransmissionFactor);
			GetTexAndSampId(extVals,
							textures,
							"transmissionTexture",
							m_Data.m_TransmissionTextureIndex,
							m_Data.m_TransmissionSamplerIndex);
		}

		// KHR_materials_ior
		extentionIter = mat.extensions.find("KHR_materials_ior");
		if (extentionIter != mat.extensions.end())
		{
			const auto& extVals = extentionIter->second;
			GetFloat(extVals, "ior", m_Data.m_Ior);
		}

		// KHR_materials_volume
		extentionIter = mat.extensions.find("KHR_materials_volume");
		if (extentionIter != mat.extensions.end())
		{
			const auto& extVals = extentionIter->second;
			GetFloat(extVals, "attenuationDistance", m_Data.m_AttenuationDistance);
			GetVec3(extVals, "attenuationColor", m_Data.m_AttenuationColor);
		};

		// KHR_materials_emissive_strength
		extentionIter = mat.extensions.find("KHR_materials_emissive_strength");
		if (extentionIter != mat.extensions.end())
		{
			const auto& extVals = extentionIter->second;
			GetFloat(extVals, "emissiveStrength", m_Data.m_EmissiveStrength);
		}
	}

} // namespace Ball