// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "VulkanShader.h"

#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GLSL.std.450.h>
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>
#include <StandAlone/ResourceLimits.h>
#include <glslang/Include/ShHandle.h>
#include <glslang/OSDependent/osinclude.h>

#include "VulkanLogger.h"
#include "IRUniformsLocator.h"

#include <cstdio>			// printf, fprintf
#include <cstdlib>			// abort
#include <iostream>			// std::cout
#include <stdexcept>		// std::exception
#include <algorithm>		// std::min, std::max
#include <fstream>			// std::ifstream
#include <chrono>			// timer

#define TRACE_MEMORY
#include <vkProfiler/Profiler.h>

//#define VERBOSE_DEBUG

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

VulkanShader::ShaderMessagingFunction s_ShaderMessagingFunction = 0;

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

VulkanShader::VulkanShader()
{
	ZoneScoped;
}

VulkanShader::~VulkanShader()
{
	ZoneScoped;
}

void VulkanShader::Init()
{
	ZoneScoped;

	glslang::InitializeProcess();
}

void VulkanShader::Unit()
{
	ZoneScoped;

	glslang::FinalizeProcess();
}

std::string GetSuffix(const std::string& name)
{
	ZoneScoped;

	const size_t pos = name.rfind('.');
	return (pos == std::string::npos) ? "" : name.substr(name.rfind('.') + 1);
}

EShLanguage GetShaderStage(const std::string& stage)
{
	ZoneScoped;

	if (stage == "vert") {
		return EShLangVertex;
	}
	else if (stage == "tesc") {
		return EShLangTessControl;
	}
	else if (stage == "tese") {
		return EShLangTessEvaluation;
	}
	else if (stage == "geom") {
		return EShLangGeometry;
	}
	else if (stage == "frag") {
		return EShLangFragment;
	}
	else if (stage == "comp") {
		return EShLangCompute;
	}
	else {
		assert(0 && "Unknown shader stage");
		return EShLangCount;
	}
}

std::string GetShaderStageString(const EShLanguage& stage)
{
	ZoneScoped;

	switch (stage)
	{
	case EShLangVertex: return "vert";
	case EShLangTessControl: return "tesc";
	case EShLangTessEvaluation: return "tese";
	case EShLangGeometry: return "geom";
	case EShLangFragment: return "frag";
	case EShLangCompute: return "comp";
	}
	return "";
}

std::string GetFullShaderStageString(const EShLanguage& stage)
{
	ZoneScoped;

	switch (stage)
	{
	case EShLangVertex: return "Vertex";
	case EShLangTessControl: return "Tesselation Control";
	case EShLangTessEvaluation: return "Tesselation Evaluation";
	case EShLangGeometry: return "Geometry";
	case EShLangFragment: return "Fragment";
	case EShLangCompute: return "Compute";
	}
	return "";
}

//TODO: Multithread, manage SpirV that doesn't need recompiling (only recompile when dirty)
const std::vector<unsigned int> VulkanShader::CompileGLSLFile(
	const std::string& filename, 
	ShaderMessagingFunction vMessagingFunction,
	std::string *vShaderCode,
	std::unordered_map<std::string, bool> *vUsedUniforms)
{
	ZoneScoped;
	
	std::vector<unsigned int> SpirV;

	//Load GLSL into a string
	std::ifstream file(filename);

	if (!file.is_open())
	{
		LogVarDebug("Failed to load shader %s", filename.c_str());
		return SpirV;
	}

	std::string InputGLSL((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());

	file.close();

	if (!InputGLSL.empty())
	{
		if (vShaderCode)
			*vShaderCode = InputGLSL;

		return CompileGLSLString(InputGLSL, GetSuffix(filename), filename, vMessagingFunction, vShaderCode, vUsedUniforms);
	}

	return SpirV;
}

//TODO: Multithread, manage SpirV that doesn't need recompiling (only recompile when dirty)
const std::vector<unsigned int> VulkanShader::CompileGLSLString(
	const std::string& vCode,
	std::string vShaderSuffix, 
	const std::string& vOriginalFileName, 
	ShaderMessagingFunction vMessagingFunction,
	std::string *vShaderCode,
	std::unordered_map<std::string, bool> *vUsedUniforms)
{
	ZoneScoped;

	LogVarDebug("==== VulkanShader::CompileGLSLString (%s) =====", vShaderSuffix.c_str());
	
	m_Error.clear();
	m_Warnings.clear();

	std::vector<unsigned int> SpirV;

	std::string InputGLSL = vCode;

	EShLanguage shaderType = GetShaderStage(vShaderSuffix);
	
	if (!InputGLSL.empty() && shaderType != EShLanguage::EShLangCount)
	{
		if (vShaderCode)
			*vShaderCode = InputGLSL;

		const char* InputCString = InputGLSL.c_str();

		glslang::TShader Shader(shaderType);
		Shader.setStrings(&InputCString, 1);

		//Set up Vulkan/SpirV Environment
		int ClientInputSemanticsVersion = 100; // maps to, say, #define VULKAN 100
		glslang::EShTargetClientVersion VulkanClientVersion = glslang::EShTargetVulkan_1_0;  // would map to, say, Vulkan 1.0
		glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_0;    // maps to, say, SPIR-V 1.0

		Shader.setEnvInput(glslang::EShSourceGlsl, shaderType, glslang::EShClientVulkan, ClientInputSemanticsVersion);
		Shader.setEnvClient(glslang::EShClientVulkan, VulkanClientVersion);
		Shader.setEnvTarget(glslang::EShTargetSpv, TargetVersion);
		Shader.setEntryPoint("main");

		EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

		const int DefaultVersion = 110; // 110 for desktop, 100 for es

		DirStackFileIncluder Includer;

		std::string PreprocessedGLSL;
		
		std::string shaderTypeString = GetFullShaderStageString(shaderType);

		if (!Shader.preprocess(&glslang::DefaultTBuiltInResource, DefaultVersion, ENoProfile, false, false, messages, &PreprocessedGLSL, Includer))
		{
			LogVarDebug("GLSL stage %s Preprocessing Failed for : %s", vShaderSuffix.c_str(), vOriginalFileName.c_str());
			LogVarDebug("%s", Shader.getInfoLog());
			LogVarDebug("%s", Shader.getInfoDebugLog());

			std::string log = Shader.getInfoLog();
			if (!log.empty())
			{
				m_Error[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Preprocessing Errors", shaderTypeString, log);
				}
			}
#ifdef VERBOSE_DEBUG
			log = Shader.getInfoDebugLog();
			if (!log.empty())
			{
				m_Error[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Preprocessing Errors", shaderTypeString, log);
				}
			}
#endif
			m_Warnings.clear();

			LogVarDebug("==========================================");

			return SpirV;
		}
		else
		{
			m_Error.clear();
			std::string log = Shader.getInfoLog();
			if (!log.empty())
			{
				m_Warnings[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Preprocessing Warnings", shaderTypeString, log);
				}
			}
#ifdef VERBOSE_DEBUG
			log = Shader.getInfoDebugLog();
			if (!log.empty())
			{
				m_Warnings[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Preprocessing Warnings", shaderTypeString, log);
				}
			}
#endif
		}

		const char* PreprocessedCStr = PreprocessedGLSL.c_str();
		Shader.setStrings(&PreprocessedCStr, 1);

		if (!Shader.parse(&glslang::DefaultTBuiltInResource, 100, false, messages))
		{
			LogVarDebug("GLSL stage %s Parse Failed for stage : %s", vShaderSuffix.c_str(), vOriginalFileName.c_str());
			LogVarDebug("%s", Shader.getInfoLog());
			LogVarDebug("%s", Shader.getInfoDebugLog());

			std::string log = Shader.getInfoLog();
			if (!log.empty())
			{
				m_Error[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Parse Errors", shaderTypeString, log);
				}
			}
#ifdef VERBOSE_DEBUG
			log = Shader.getInfoDebugLog();
			if (!log.empty())
			{
				m_Error[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Parse Errors", shaderTypeString, log);
				}
			}
#endif
			m_Warnings.clear();

			LogVarDebug("==========================================");

			return SpirV;
		}
		else
		{
			m_Error.clear();
			std::string log = Shader.getInfoLog();
			if (!log.empty())
			{
				m_Warnings[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Parse Warnings", shaderTypeString, log);
				}
			}
#ifdef VERBOSE_DEBUG
			log = Shader.getInfoDebugLog();
			if (!log.empty())
			{
				m_Warnings[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Parse Warnings", shaderTypeString, log);
				}
			}
#endif
		}
		
		glslang::TProgram Program;
		Program.addShader(&Shader);

		if (!Program.link(messages))
		{
			LogVarDebug("GLSL stage %s Linking Failed for : %s", vShaderSuffix.c_str(), vOriginalFileName.c_str());
			LogVarDebug("%s", Shader.getInfoLog());
			LogVarDebug("%s", Shader.getInfoDebugLog());

			std::string log = Shader.getInfoLog();
			if (!log.empty())
			{
				m_Error[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Linking Errors", shaderTypeString, log);
				}
			}
#ifdef VERBOSE_DEBUG
			log = Shader.getInfoDebugLog();
			if (!log.empty())
			{
				m_Error[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Linking Errors", shaderTypeString, log);
				}
			}
#endif
			m_Warnings.clear();

			LogVarDebug("==========================================");

			return SpirV;
		}
		else
		{
			m_Error.clear();
			std::string log = Shader.getInfoLog();
			if (!log.empty())
			{
				m_Warnings[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Linking Warnings", shaderTypeString, log);
				}
			}
#ifdef VERBOSE_DEBUG
			log = Shader.getInfoDebugLog();
			if (!log.empty())
			{
				m_Warnings[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Linking Warnings", shaderTypeString, log);
				}
			}
#endif
		}

		if (vUsedUniforms)
		{
			auto usedUniforms = CollectUniformInfosFromIR(*Shader.getIntermediate());
			for (auto u : usedUniforms)
			{
				(*vUsedUniforms)[u.first] |= u.second;
			}
		}
		
		spv::SpvBuildLogger logger;
		glslang::SpvOptions spvOptions;
		spvOptions.optimizeSize = true;
		spvOptions.stripDebugInfo = true;
		glslang::GlslangToSpv(*Program.getIntermediate(shaderType), SpirV, &logger, &spvOptions);

		if (logger.getAllMessages().length() > 0)
		{
			std::string allmsgs = logger.getAllMessages();
			std::cout << allmsgs << std::endl;
		}
	}

	if (SpirV.empty())
	{
		LogVarDebug("Shader stage %s Spirv generation of %s : NOK !", vShaderSuffix.c_str(), vOriginalFileName.c_str());
	}
	else
	{
		LogVarDebug("Shader stage %s Spirv generation of %s : OK !", vShaderSuffix.c_str(), vOriginalFileName.c_str());
	}

	LogVarDebug("==========================================");

	return SpirV;
}

void VulkanShader::ParseGLSLString(
	const std::string& vCode,
	std::string vShaderSuffix,
	const std::string& vOriginalFileName,
	std::string vEntryPoint,
	ShaderMessagingFunction vMessagingFunction,
	TraverserFunction vTraverser)
{
	ZoneScoped;

	std::string InputGLSL = vCode;

	EShLanguage shaderType = GetShaderStage(vShaderSuffix);

	if (!InputGLSL.empty() && shaderType != EShLanguage::EShLangCount)
	{
		const char* InputCString = InputGLSL.c_str();

		glslang::TShader Shader(shaderType);
		Shader.setStrings(&InputCString, 1);

		//Set up Vulkan/SpirV Environment
		int ClientInputSemanticsVersion = 100; // maps to, say, #define VULKAN 100
		glslang::EShTargetClientVersion VulkanClientVersion = glslang::EShTargetVulkan_1_0;  // would map to, say, Vulkan 1.0
		glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_0;    // maps to, say, SPIR-V 1.0

		Shader.setEnvInput(glslang::EShSourceGlsl, shaderType, glslang::EShClientVulkan, ClientInputSemanticsVersion);
		Shader.setEnvClient(glslang::EShClientVulkan, VulkanClientVersion);
		Shader.setEnvTarget(glslang::EShTargetSpv, TargetVersion);
		Shader.setEntryPoint(vEntryPoint.c_str());

		EShMessages messages = (EShMessages)(EShMsgAST);

		const int DefaultVersion = 110; // 110 for desktop, 100 for es

		DirStackFileIncluder Includer;

		std::string PreprocessedGLSL;

		std::string shaderTypeString = GetFullShaderStageString(shaderType);

		if (!Shader.preprocess(&glslang::DefaultTBuiltInResource, DefaultVersion, ENoProfile, false, false, messages, &PreprocessedGLSL, Includer))
		{
			LogVarDebug("GLSL stage %s Preprocessing Failed for : %s", vShaderSuffix.c_str(), vOriginalFileName.c_str());
			LogVarDebug("%s", Shader.getInfoLog());
			LogVarDebug("%s", Shader.getInfoDebugLog());

			std::string log = Shader.getInfoLog();
			if (!log.empty())
			{
				m_Error[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Preprocessing Errors", shaderTypeString, log);
				}
			}
#ifdef VERBOSE_DEBUG
			log = Shader.getInfoDebugLog();
			if (!log.empty())
			{
				m_Error[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Preprocessing Errors", shaderTypeString, log);
				}
			}
#endif
			m_Warnings.clear();

			LogVarDebug("==========================================");
		}
		else
		{
			m_Error.clear();
			std::string log = Shader.getInfoLog();
			if (!log.empty())
			{
				m_Warnings[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Preprocessing Warnings", shaderTypeString, log);
				}
			}
#ifdef VERBOSE_DEBUG
			log = Shader.getInfoDebugLog();
			if (!log.empty())
			{
				m_Warnings[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Preprocessing Warnings", shaderTypeString, log);
				}
			}
#endif
		}
		
		const char* PreprocessedCStr = PreprocessedGLSL.c_str();
		Shader.setStrings(&PreprocessedCStr, 1);

		if (!Shader.parse(&glslang::DefaultTBuiltInResource, 100, false, messages))
		{
			LogVarDebug("GLSL stage %s Parse Failed for stage : %s", vShaderSuffix.c_str(), vOriginalFileName.c_str());
			LogVarDebug("%s", Shader.getInfoLog());
			LogVarDebug("%s", Shader.getInfoDebugLog());

			std::string log = Shader.getInfoLog();
			if (!log.empty())
			{
				m_Error[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Parse Errors", shaderTypeString, log);
				}
			}
#ifdef VERBOSE_DEBUG
			log = Shader.getInfoDebugLog();
			if (!log.empty())
			{
				m_Error[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Parse Errors", shaderTypeString, log);
				}
			}
#endif
			m_Warnings.clear();

			LogVarDebug("==========================================");
		}
		else
		{
			m_Error.clear();
			std::string log = Shader.getInfoLog();
			if (!log.empty())
			{
				m_Warnings[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Parse Warnings", shaderTypeString, log);
				}
			}
#ifdef VERBOSE_DEBUG
			log = Shader.getInfoDebugLog();
			if (!log.empty())
			{
				m_Warnings[shaderType].push_back(log);
				if (vMessagingFunction)
				{
					vMessagingFunction("Parse Warnings", shaderTypeString, log);
				}
			}
#endif
		}

		if (vTraverser)
		{
			vTraverser(Shader.getIntermediate());
		}
	}
}

VkShaderModule VulkanShader::CreateShaderModule(VkDevice vLogicalDevice, std::vector<unsigned int> vSPIRVCode)
{
	ZoneScoped;

	VkShaderModule shaderModule = 0;

	if (!vSPIRVCode.empty())
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = vSPIRVCode.size() * sizeof(unsigned int);
		createInfo.pCode = vSPIRVCode.data();

		if (vkCreateShaderModule(vLogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			LogVarDebug("fail to create shader module !");
			shaderModule = 0;
		}
	}
	else
	{
		LogVarDebug("SPIRV Code is empty. Fail to create shader module !");
		shaderModule = 0;
	}

	if (shaderModule)
	{
		//LogVarDebug("shader module compilation : OK !");
	}

	return shaderModule;
}

void VulkanShader::DestroyShaderModule(VkDevice vLogicalDevice, VkShaderModule vVkShaderModule)
{
	ZoneScoped;

	vkDestroyShaderModule(vLogicalDevice, vVkShaderModule, nullptr);
}

std::unordered_map<std::string, bool> VulkanShader::CollectUniformInfosFromIR(const glslang::TIntermediate& intermediate)
{
	ZoneScoped;

	std::unordered_map<std::string, bool> res;

	TIntermNode* root = intermediate.getTreeRoot();

	if (root == 0)
		return res;

	TIRUniformsLocator it;
	root->traverse(&it);
	res = it.usedUniforms;

	return res;
}