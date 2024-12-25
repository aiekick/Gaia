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

#pragma once
#pragma warning(disable : 4251)

#include <vulkan/vulkan.hpp>
#include <Gaia/Resources/VulkanRessource.h>
#include <ezlibs/ezLog.hpp>

#include <map>
#include <string>
#include <vector>
#include <unordered_map>

// #define PRINT_BLOCK_DATAS

/*
manage in auto the alignement of Buffer for the standard std430 only
*/

class GAIA_API StorageBufferStd430 {
private:
    // key = uniform name, offset in buffer for have op on uniform data
    std::unordered_map<std::string, uint32_t> offsets;
    // uniforms datas buffer
    std::vector<uint8_t> datas;

    bool firstUploadWasDone = false;

    // if he is dirty, value has been changed and mut be uploaded in gpu memory
    // dirty at first for init in gpu memory
    bool isDirty = true;

    // if new var as added, need recreation, because there is a new size
    bool needRecreation = false;

private:  // custom Buffer Info
    bool customBufferInfo = false;

public:  // vulkan object to share
    VulkanBufferObjectPtr bufferObjectPtr = nullptr;
    vk::DescriptorBufferInfo descriptorBufferInfo = {VK_NULL_HANDLE, 0, VK_WHOLE_SIZE};
    VmaMemoryUsage memoryUsage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;

public:
    StorageBufferStd430() = default;
    ~StorageBufferStd430();

    // init / unit
    bool Build();
    void Unit();
    void Clear();
    void SetDirty();  // say => there is some change to upload on the gpu
    bool IsDirty();   // say => is needed to be uploaded
    bool IsOk();      // was uploaded one time at least, so the vkBuffer is valid to bind

    // custom Buffer Info
    void UseCustomBufferInfo();
    void SetCustomBufferInfo(vk::DescriptorBufferInfo* vBufferObjectInfo);

    // upload to gpu memory
    void Upload(GaiApi::VulkanCoreWeak vVulkanCore, bool vOnlyIfDirty);

    // create/destory ubo
    bool CreateSBO(GaiApi::VulkanCoreWeak vVulkanCore, VmaMemoryUsage vVmaMemoryUsage);
    void DestroySBO();
    bool RecreateSBO(GaiApi::VulkanCoreWeak vVulkanCore);

    // add size to uniform block, return startOffset
    bool RegisterByteSize(const std::string& vKey, uint32_t vSizeInBytes, uint32_t* vStartOffset = 0);

    // templates (defined under class)
    // add a variable
    template <typename T>
    void RegisterVar(const std::string& vKey, T vValue);  // add var to uniform block
    template <typename T>
    void RegisterVar(const std::string& vKey, T* vValue, uint32_t vSizeInBytes);  // add var to uniform block

    // Get / set + op on variables
    template <typename T>
    bool GetVar(const std::string& vKey, T& vValue);  // Get
    template <typename T>
    bool SetVar(const std::string& vKey, T vValue);  // set
    template <typename T>
    bool SetVar(const std::string& vKey, T* vValue, uint32_t vSizeInBytes);  // set
    template <typename T>
    bool SetAddVar(const std::string&, T vValue);  // add and set like +=

private:
    bool OffsetExist(const std::string& vKey);
    uint32_t GetGoodAlignement(uint32_t vSize);
    void AddOffsetForKey(const std::string& vKey, uint32_t vOffset);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PUBLIC TEMPLATES //////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
void StorageBufferStd430::RegisterVar(const std::string& vKey, T* vValue, uint32_t vSizeInBytes) {
    uint32_t startOffset;
    if (RegisterByteSize(vKey, vSizeInBytes, &startOffset)) {
        // on copy de "startOffset" � "startOffset + vSizeInBytes"
        memcpy(datas.data() + startOffset, vValue, vSizeInBytes);
    }
}

template <typename T>
void StorageBufferStd430::RegisterVar(const std::string& vKey, T vValue) {
    RegisterVar(vKey, &vValue, sizeof(vValue));
}

template <typename T>
bool StorageBufferStd430::GetVar(const std::string& vKey, T& vValue) {
    if (OffsetExist(vKey)) {
        uint32_t offset = offsets[vKey];
        uint32_t size = sizeof(vValue);
        memcpy(&vValue, datas.data() + offset, size);
        return true;
    }
    LogVarDebugInfo("key %s not exist in UniformBlockStd140. GetVar fail.", vKey.c_str());
    return false;
}

template <typename T>
bool StorageBufferStd430::SetVar(const std::string& vKey, T* vValue, uint32_t vSizeInBytes) {
    if (OffsetExist(vKey) && vSizeInBytes > 0) {
        uint32_t newSize = vSizeInBytes;
        uint32_t offset = offsets[vKey];
        memcpy(datas.data() + offset, vValue, newSize);
        isDirty = true;
        return true;
    }
    LogVarDebugInfo("key %s not exist in UniformBlockStd140. SetVar fail.", vKey.c_str());
    return false;
}

template <typename T>
bool StorageBufferStd430::SetVar(const std::string& vKey, T vValue) {
    return SetVar(vKey, &vValue, sizeof(vValue));
}

template <typename T>
bool StorageBufferStd430::SetAddVar(const std::string& vKey, T vValue) {
    T v;
    if (GetVar(vKey, v)) {
        v += vValue;
        return SetVar(vKey, v);
    }
    LogVarDebugInfo("key %s not exist in UniformBlockStd140. SetAddVar fail.", vKey.c_str());
    return false;
}