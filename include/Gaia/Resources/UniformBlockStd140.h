#pragma once
#pragma warning(disable : 4251)

#include <vulkan/vulkan.hpp>
#include <Gaia/Resources/VulkanRessource.h>
#include <ctools/Logger.h>

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

// #define PRINT_BLOCK_DATAS

/*
manage in auto the alignement of Uniform Buffer for the standard std140 only
*/

class GAIA_API UniformBlockStd140 {
private:
    // key = uniform name, offset in buffer for have op on uniform data
    std::unordered_map<std::string, uint32_t> offsets;
    // uniforms datas buffer
    std::vector<uint8_t> datas;

    // if he is dirty, value has been changed and mut be uploaded in gpu memory
    // dirty at first for init in gpu memory
    bool isDirty = true;

private:  // custom Buffer Info
    bool customBufferInfo = false;

public:  // vulkan object to share
    std::shared_ptr<VulkanBufferObject> bufferObjectPtr = nullptr;
    vk::DescriptorBufferInfo descriptorBufferInfo = {VK_NULL_HANDLE, 0, VK_WHOLE_SIZE};

public:
    ~UniformBlockStd140();

    // init / unit
    bool Build();
    void Unit();
    void Clear();
    void SetDirty();

    // custom Buffer Info
    void UseCustomBufferInfo();
    void SetCustomBufferInfo(vk::DescriptorBufferInfo* vBufferObjectInfo);

    // upload to gpu memory
    void Upload(GaiApi::VulkanCoreWeak vVulkanCore, bool vOnlyIfDirty);

    // create/destory ubo
    bool CreateUBO(GaiApi::VulkanCoreWeak vVulkanCore);
    void DestroyUBO();
    bool RecreateUBO(GaiApi::VulkanCoreWeak vVulkanCore);

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
void UniformBlockStd140::RegisterVar(const std::string& vKey, T* vValue, uint32_t vSizeInBytes) {
    uint32_t startOffset;
    if (RegisterByteSize(vKey, vSizeInBytes, &startOffset)) {
        // on copy de "startOffset" a "startOffset + vSizeInBytes"
        memcpy(datas.data() + startOffset, vValue, vSizeInBytes);
    }
}

template <typename T>
void UniformBlockStd140::RegisterVar(const std::string& vKey, T vValue) {
    RegisterVar(vKey, &vValue, sizeof(vValue));
}

template <typename T>
bool UniformBlockStd140::GetVar(const std::string& vKey, T& vValue) {
    if (OffsetExist(vKey)) {
        uint32_t offset = offsets[vKey];
        uint32_t size = sizeof(vValue);
        memcpy(&vValue, datas.data() + offset, size);
        return true;
    }
    LogVarDebugInfo("Debug : key %s not exist in UniformBlockStd140. GetVar fail.", vKey.c_str());
    return false;
}

template <typename T>
bool UniformBlockStd140::SetVar(const std::string& vKey, T* vValue, uint32_t vSizeInBytes) {
    if (OffsetExist(vKey) && vSizeInBytes > 0) {
        uint32_t newSize = vSizeInBytes;
        uint32_t offset = offsets[vKey];
        memcpy(datas.data() + offset, vValue, newSize);
        isDirty = true;
        return true;
    }
    LogVarDebugInfo("Debug : key %s not exist in UniformBlockStd140. SetVar fail.", vKey.c_str());
    return false;
}

template <typename T>
bool UniformBlockStd140::SetVar(const std::string& vKey, T vValue) {
    return SetVar(vKey, &vValue, sizeof(vValue));
}

template <typename T>
bool UniformBlockStd140::SetAddVar(const std::string& vKey, T vValue) {
    T v;
    if (GetVar(vKey, v)) {
        v += vValue;
        return SetVar(vKey, v);
    }
    LogVarDebugInfo("Debug : key %s not exist in UniformBlockStd140. SetAddVar fail.", vKey.c_str());
    return false;
}
