#include "lve_pipeline.hpp"

#include <fstream>
#include <iostream>
namespace lve
{
	LvePipeline::LvePipeline(LveDevice& device,
		const std::string& vertFilePath, const std::string& fragFilePath,
		const PipelineConfigInfo& configInfo)
		: lveDevice(device)
	{
		createGraphicsPipeline(vertFilePath, fragFilePath, configInfo);
	}

	PipelineConfigInfo LvePipeline::defaultPipelineConfigInfo(uint32_t width, uint32_t height)
	{
		return PipelineConfigInfo();
	}

	std::vector<char> LvePipeline::readFile(const std::string& filePath)
	{
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);
		if (!file.is_open())
			throw std::runtime_error("Failed to open file: " + filePath);

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		
		file.close();
		return buffer;
	}

	void LvePipeline::createGraphicsPipeline(
		const std::string& vertFilePath, const std::string& fragFilePath,
		const PipelineConfigInfo& configInfo)
	{
		auto vertCode = readFile(vertFilePath);
		auto fragCode = readFile(fragFilePath);

		std::cout << "Vertex Shader code size: " << vertCode.size() << "\n";
		std::cout << "Fragment Shader code size: " << fragCode.size() << "\n";

	}

	void LvePipeline::createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		if (vkCreateShaderModule(lveDevice.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS)
			throw std::runtime_error("Failed to create Shader Module");
	}

}
