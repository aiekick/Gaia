#include <Gaia/Utils/LoggingUtils.h>

namespace GaiApi {

const char* LoggingUtils::DescriptorTypeToString(const vk::DescriptorType& vType) {
    switch (vType) {
        case vk::DescriptorType::eSampler: return "eSampler"; break;
        case vk::DescriptorType::eCombinedImageSampler: return "eCombinedImageSampler"; break;
        case vk::DescriptorType::eSampledImage: return "eSampledImage"; break;
        case vk::DescriptorType::eStorageImage: return "eStorageImage"; break;
        case vk::DescriptorType::eUniformTexelBuffer: return "eUniformTexelBuffer"; break;
        case vk::DescriptorType::eStorageTexelBuffer: return "eStorageTexelBuffer"; break;
        case vk::DescriptorType::eUniformBuffer: return "eUniformBuffer"; break;
        case vk::DescriptorType::eStorageBuffer: return "eStorageBuffer"; break;
        case vk::DescriptorType::eUniformBufferDynamic: return "eUniformBufferDynamic"; break;
        case vk::DescriptorType::eStorageBufferDynamic: return "eStorageBufferDynamic"; break;
        case vk::DescriptorType::eInputAttachment: return "eInputAttachment"; break;
        case vk::DescriptorType::eInlineUniformBlock: return "eInlineUniformBlock"; break;
        case vk::DescriptorType::eAccelerationStructureKHR: return "eAccelerationStructureKHR"; break;
        default: break;
    }
    return "";
}

const std::string LoggingUtils::ShaderStageFlagsToString(const vk::ShaderStageFlags& vFlags) {
    std::string shaderStageFlags;
    if (vFlags & vk::ShaderStageFlagBits::eVertex) {
        shaderStageFlags = "eVertex";
    }
    if (vFlags & vk::ShaderStageFlagBits::eTessellationControl) {
        if (!shaderStageFlags.empty()) {
            shaderStageFlags += " | ";
        }
        shaderStageFlags += "eTessellationControl";
    }
    if (vFlags & vk::ShaderStageFlagBits::eTessellationEvaluation) {
        if (!shaderStageFlags.empty()) {
            shaderStageFlags += " | ";
        }
        shaderStageFlags += "eTessellationEvaluation";
    }
    if (vFlags & vk::ShaderStageFlagBits::eGeometry) {
        if (!shaderStageFlags.empty()) {
            shaderStageFlags += " | ";
        }
        shaderStageFlags += "eGeometry";
    }
    if (vFlags & vk::ShaderStageFlagBits::eFragment) {
        if (!shaderStageFlags.empty()) {
            shaderStageFlags += " | ";
        }
        shaderStageFlags += "eFragment";
    }
    if (vFlags & vk::ShaderStageFlagBits::eCompute) {
        if (!shaderStageFlags.empty()) {
            shaderStageFlags += " | ";
        }
        shaderStageFlags += "eCompute";
    }
    if (vFlags & vk::ShaderStageFlagBits::eRaygenKHR) {
        if (!shaderStageFlags.empty()) {
            shaderStageFlags += " | ";
        }
        shaderStageFlags += "eRaygenKHR";
    }
    if (vFlags & vk::ShaderStageFlagBits::eAnyHitKHR) {
        if (!shaderStageFlags.empty()) {
            shaderStageFlags += " | ";
        }
        shaderStageFlags += "eAnyHitKHR";
    }
    if (vFlags & vk::ShaderStageFlagBits::eClosestHitKHR) {
        if (!shaderStageFlags.empty()) {
            shaderStageFlags += " | ";
        }
        shaderStageFlags += "eClosestHitKHR";
    }
    if (vFlags & vk::ShaderStageFlagBits::eMissKHR) {
        if (!shaderStageFlags.empty()) {
            shaderStageFlags += " | ";
        }
        shaderStageFlags += "eMissKHR";
    }
    if (vFlags & vk::ShaderStageFlagBits::eIntersectionKHR) {
        if (!shaderStageFlags.empty()) {
            shaderStageFlags += " | ";
        }
        shaderStageFlags += "eIntersectionKHR";
    }
    if (vFlags & vk::ShaderStageFlagBits::eCallableKHR) {
        if (!shaderStageFlags.empty()) {
            shaderStageFlags += " | ";
        }
        shaderStageFlags += "eCallableKHR";
    }
    return shaderStageFlags;
}

}  // namespace GaiApi
