#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <glslang/glslang/MachineIndependent/localintermediate.h>

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <array>

class VulkanShader
{
public:
	typedef std::function<void(std::string, std::string, std::string)> ShaderMessagingFunction;
	typedef std::function<void(glslang::TIntermediate*)> TraverserFunction;

public:
	const std::vector<unsigned int> CompileGLSLFile(
		const std::string& filename, 
		ShaderMessagingFunction vMessagingFunction,
		std::string *vShaderCode = 0,
		std::unordered_map<std::string, bool> *vUsedUniforms = 0);
	const std::vector<unsigned int> CompileGLSLString(
		const std::string& vCode, 
		std::string vShaderSuffix, 
		const std::string& vOriginalFileName,
		ShaderMessagingFunction vMessagingFunction,
		std::string *vShaderCode = 0, 
		std::unordered_map<std::string, bool> *vUsedUniforms = 0);
	void VulkanShader::ParseGLSLString(
		const std::string& vCode,
		std::string vShaderSuffix,
		const std::string& vOriginalFileName,
		std::string vEntryPoint,
		ShaderMessagingFunction vMessagingFunction,
		TraverserFunction vTraverser);
	VkShaderModule CreateShaderModule(VkDevice vLogicalDevice, std::vector<unsigned int> vSPIRVCode);
	void DestroyShaderModule(VkDevice vLogicalDevice, VkShaderModule vVkShaderModule);
	std::unordered_map<std::string, bool> CollectUniformInfosFromIR(const glslang::TIntermediate& intermediate);

public: // errors
	std::unordered_map<EShLanguage, std::vector<std::string>> m_Error;
	std::unordered_map<EShLanguage, std::vector<std::string>> m_Warnings;

public: // singleton
	static VulkanShader *Instance()
	{
		static VulkanShader _instance;
		return &_instance;
	}

protected:
	VulkanShader(); // Prevent construction
	VulkanShader(const VulkanShader&) {}; // Prevent construction by copying
	VulkanShader& operator =(const VulkanShader&) { return *this; }; // Prevent assignment
	~VulkanShader(); // Prevent unwanted destruction

public:
	void Init();
	void Unit();
};
