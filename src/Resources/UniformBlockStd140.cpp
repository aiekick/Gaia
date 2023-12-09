// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <Gaia/Resources/UniformBlockStd140.h>

#ifdef PROFILER_INCLUDE
#include <vulkan/vulkan.hpp>
#include PROFILER_INCLUDE
#endif
#ifndef ZoneScoped
#define ZoneScoped
#endif

using namespace GaiApi;

UniformBlockStd140::~UniformBlockStd140() {
    ZoneScoped;
    Unit();
}

bool UniformBlockStd140::Build() {
    ZoneScoped;
    return true;
}

void UniformBlockStd140::Unit() {
    ZoneScoped;
    DestroyUBO();
    Clear();
}

void UniformBlockStd140::Clear() {
    ZoneScoped;
    descriptorBufferInfo = vk::DescriptorBufferInfo{VK_NULL_HANDLE, 0, VK_WHOLE_SIZE};
    datas.clear();
    offsets.clear();
    isDirty = false;
}

void UniformBlockStd140::SetDirty() {
    ZoneScoped;
    isDirty = true;
}

void UniformBlockStd140::UseCustomBufferInfo() {
    ZoneScoped;
    customBufferInfo = true;
}

void UniformBlockStd140::SetCustomBufferInfo(vk::DescriptorBufferInfo* vBufferObjectInfo) {
    ZoneScoped;
    if (vBufferObjectInfo) {
        customBufferInfo = true;
        descriptorBufferInfo = *vBufferObjectInfo;
    }
}

void UniformBlockStd140::Upload(GaiApi::VulkanCoreWeak vVulkanCore, bool vOnlyIfDirty) {
    ZoneScoped;
    if (!vOnlyIfDirty || (vOnlyIfDirty && isDirty)) {
        if (bufferObjectPtr && !customBufferInfo) {
            VulkanRessource::upload(vVulkanCore, bufferObjectPtr, datas.data(), datas.size());
        }
        isDirty = false;
    }
}

bool UniformBlockStd140::CreateUBO(GaiApi::VulkanCoreWeak vVulkanCore) {
    ZoneScoped;
    if (customBufferInfo) {
        if (!descriptorBufferInfo.buffer)  // si le buffer est vide alors on va l'init avec un buffer de taille 1
        {
            // on init le buffer avec un buffer de taille 1
            // pour eviter une erreur avec l'init du descriptor avec un buffer null
            // qui peut crasher l'application
            datas.push_back(1U);
            // ca ne change rien pour apres puisque qu'on bindera un buffer externe
            // ca va rserver une adresse en sommes
        } else {
            return true;
        }
    }
    if (!datas.empty()) {
        bufferObjectPtr = VulkanRessource::createUniformBufferObject(vVulkanCore, datas.size(), "UniformBlockStd140");
        if (bufferObjectPtr) {
            descriptorBufferInfo.buffer = bufferObjectPtr->buffer;
            descriptorBufferInfo.range = datas.size();
            descriptorBufferInfo.offset = 0;
            return true;
        }
    } else {
        LogVarDebugInfo("Debug : CreateUBO() Fail, datas is empty, nothing to upload !");
    }
    return false;
}

void UniformBlockStd140::DestroyUBO() {
    ZoneScoped;
    bufferObjectPtr.reset();
}

bool UniformBlockStd140::RecreateUBO(GaiApi::VulkanCoreWeak vVulkanCore) {
    ZoneScoped;
    bool res = false;
    if (!customBufferInfo) {
        if (bufferObjectPtr) {
            DestroyUBO();
            CreateUBO(vVulkanCore);

            res = true;
        }
    } else {
        assert(0);  // c'est pas normal d'appeler RecreateUBO avec un custom
    }

    return res;
}

// add size to uniform block, return startOffset
bool UniformBlockStd140::RegisterByteSize(const std::string& vKey, uint32_t vSizeInBytes, uint32_t* vStartOffset) {
    ZoneScoped;
    if (OffsetExist(vKey)) {
        LogVarDebugWarning("Debug : key %s is already defined in UniformBlockStd140. RegisterVar fail.", vKey.c_str());
    } else if (vSizeInBytes > 0) {
        uint32_t newSize = vSizeInBytes;
        uint32_t lastOffset = (uint32_t)datas.size();
        auto baseAlign = GetGoodAlignement(newSize);
        // il faut trouver le prochain offset qui est multiple de baseAlign
        auto startOffset = baseAlign * (uint32_t)std::ceil((double)lastOffset / (double)baseAlign);
        auto newSizeToAllocate = startOffset - lastOffset + newSize;
#ifdef PRINT_BLOCK_DATAS
        auto endOffset = startOffset + newSize;
        LogVarTag(MESSAGING_TYPE_DEBUG, "key %s, size %u, align %u, Offsets : %u => %u, size to alloc %u\n", vKey.c_str(), newSize, baseAlign,
            startOffset, endOffset, newSizeToAllocate);
#endif
        datas.resize(lastOffset + newSizeToAllocate);
        // on set de "lastOffset" � "lastOffset + newSizeToAllocate"
        memset(datas.data() + lastOffset, 0, newSizeToAllocate);
        AddOffsetForKey(vKey, startOffset);

        if (vStartOffset)
            *vStartOffset = startOffset;

        return true;
    }

    return false;
}

void UniformBlockStd140::AddOffsetForKey(const std::string& vKey, uint32_t vOffset) {
    ZoneScoped;
    offsets[vKey] = vOffset;
}

bool UniformBlockStd140::OffsetExist(const std::string& vKey) {
    ZoneScoped;
    return offsets.find(vKey) != offsets.end();
}

uint32_t UniformBlockStd140::GetGoodAlignement(uint32_t vSize) {
    ZoneScoped;
    uint32_t goodAlign = (uint32_t)std::pow(2, std::ceil(log(vSize) / log(2)));
    return std::min(goodAlign, 16u);
}