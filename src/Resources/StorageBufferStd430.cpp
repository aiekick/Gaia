/*
Copyright 2018-2022 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <Gaia/Resources/StorageBufferStd430.h>

#ifdef PROFILER_INCLUDE
#include <vulkan/vulkan.hpp>
#include PROFILER_INCLUDE
#endif
#ifndef ZoneScoped
#define ZoneScoped
#endif

using namespace GaiApi;

StorageBufferStd430::~StorageBufferStd430() {
    ZoneScoped;
    Unit();
}

bool StorageBufferStd430::Build() {
    ZoneScoped;
    return true;
}

void StorageBufferStd430::Unit() {
    ZoneScoped;
    DestroySBO();
    Clear();
}

void StorageBufferStd430::Clear() {
    ZoneScoped;
    datas.clear();
    offsets.clear();
    isDirty = false;
}

void StorageBufferStd430::SetDirty() {
    ZoneScoped;
    isDirty = true;
}

bool StorageBufferStd430::IsDirty() {
    ZoneScoped;
    return isDirty;
}

void StorageBufferStd430::UseCustomBufferInfo() {
    ZoneScoped;
    customBufferInfo = true;
}

void StorageBufferStd430::SetCustomBufferInfo(vk::DescriptorBufferInfo* vBufferObjectInfo) {
    ZoneScoped;
    if (vBufferObjectInfo) {
        customBufferInfo = true;
        descriptorBufferInfo = *vBufferObjectInfo;
    }
}

bool StorageBufferStd430::IsOk() {
    ZoneScoped;
    return firstUploadWasDone;
}

void StorageBufferStd430::Upload(GaiApi::VulkanCoreWeak vVulkanCore, bool vOnlyIfDirty) {
    ZoneScoped;
    if (!vVulkanCore.expired()) {
        RecreateSBO(vVulkanCore);

        if (!vOnlyIfDirty || (vOnlyIfDirty && isDirty)) {
            if (bufferObjectPtr && !customBufferInfo) {
                VulkanRessource::upload(vVulkanCore, bufferObjectPtr, datas.data(), datas.size());

                firstUploadWasDone = true;
                isDirty = false;
            }
        }
    }
}

bool StorageBufferStd430::CreateSBO(GaiApi::VulkanCoreWeak vVulkanCore, VmaMemoryUsage vVmaMemoryUsage) {
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
        needRecreation = false;

        bufferObjectPtr = VulkanRessource::createStorageBufferObject(vVulkanCore, datas.size(), vVmaMemoryUsage, "StorageBufferStd430");
        if (bufferObjectPtr && bufferObjectPtr->buffer) {
            descriptorBufferInfo.buffer = bufferObjectPtr->buffer;
            descriptorBufferInfo.range = datas.size();
            descriptorBufferInfo.offset = 0;

            Upload(vVulkanCore, false);

            return true;
        } else {
            DestroySBO();
        }
    } else {
        LogVarDebugInfo("CreateSBO() Fail, datas is empty, nothing to upload !");
    }
    return false;
}

void StorageBufferStd430::DestroySBO() {
    ZoneScoped;
    bufferObjectPtr.reset();
    descriptorBufferInfo = vk::DescriptorBufferInfo{VK_NULL_HANDLE, 0, VK_WHOLE_SIZE};
}

bool StorageBufferStd430::RecreateSBO(GaiApi::VulkanCoreWeak vVulkanCore) {
    ZoneScoped;
    if (needRecreation) {
        if (!customBufferInfo) {
            if (!vVulkanCore.expired() && bufferObjectPtr) {
                VmaMemoryUsage usage = bufferObjectPtr->alloc_usage;
                DestroySBO();
                CreateSBO(vVulkanCore, usage);

                isDirty = true;

                return true;
            }
        } else {
            assert(0);  // c'est pas normal d'appeler RecreateUBO avec un custom
        }
    }
    return false;
}

bool StorageBufferStd430::RegisterByteSize(const std::string& vKey, uint32_t vSizeInBytes, uint32_t* vStartOffset) {
    ZoneScoped;
    if (OffsetExist(vKey)) {
        LogVarDebugWarning("key %s is already defined in UniformBlockStd140. RegisterVar fail.", vKey.c_str());
    } else if (vSizeInBytes > 0) {
        uint32_t newSize = vSizeInBytes;
        uint32_t lastOffset = (uint32_t)datas.size();
        auto baseAlign = GetGoodAlignement(newSize);
        // il faut trouver le prochain offset qui est multiple de baseAlign
        auto startOffset = baseAlign * (uint32_t)std::ceil((double)lastOffset / (double)baseAlign);
        auto newSizeToAllocate = startOffset - lastOffset + newSize;
#ifdef PRINT_BLOCK_DATAS
        auto endOffset = startOffset + newSize;
        LogVarTag(MESSAGING_TYPE_DEBUG, "key %s, size %u, align %u, Offsets : %u => %u, size to alloc %u", vKey.c_str(), newSize, baseAlign,
            startOffset, endOffset, newSizeToAllocate);
#endif
        datas.resize(lastOffset + newSizeToAllocate);
        // on set de "lastOffset" � "lastOffset + newSizeToAllocate"
        memset(datas.data() + lastOffset, 0, newSizeToAllocate);
        AddOffsetForKey(vKey, startOffset);

        needRecreation = true;

        if (vStartOffset)
            *vStartOffset = startOffset;

        return true;
    }

    return false;
}

void StorageBufferStd430::AddOffsetForKey(const std::string& vKey, uint32_t vOffset) {
    ZoneScoped;
    offsets[vKey] = vOffset;
}

bool StorageBufferStd430::OffsetExist(const std::string& vKey) {
    ZoneScoped;
    return offsets.find(vKey) != offsets.end();
}

uint32_t StorageBufferStd430::GetGoodAlignement(uint32_t vSize) {
    ZoneScoped;
    uint32_t goodAlign = (uint32_t)std::pow(2, std::ceil(log(vSize) / log(2)));
    return std::min(goodAlign, 16u);
}
