#pragma once
#include <string>

#include "ShaderLayout.h"
#include "TypeDefs.h"
#include "Utilities/FileWatch.h"

namespace Ball
{

	class ComputePipelineDescription : public IFileWatchListener
	{
	public:
		ComputePipelineDescription() = default;
		~ComputePipelineDescription() override{};

		void Initialize(const std::string& shaderName, ShaderLayout& layout);
		void OnFileWatchEvent(const std::string& shader) override;
		GPUComputePipelineHandle& GetPipelineHandleRef() { return m_PipelineHandle; }
		ShaderLayout GetShaderLayout() const { return m_ShaderLayout; }
		ShaderLayout& GetShaderLayoutRef() { return m_ShaderLayout; }
		std::string GetShaderName() const { return m_ShaderName; }

	private:
		ShaderLayout m_ShaderLayout;
		GPUComputePipelineHandle m_PipelineHandle;

		std::string m_ShaderName = "NOT_INITIALIZED";
	};

} // namespace Ball
