#pragma once

#include <Gaia/gaia.h>

template <typename T_VertexType>
class MeshInfo {
public:
    VulkanBufferObjectPtr m_Buffer = nullptr;
    vk::DescriptorBufferInfo m_BufferInfo = vk::DescriptorBufferInfo{VK_NULL_HANDLE, 0, VK_WHOLE_SIZE};
    std::vector<T_VertexType> m_Array;
    uint32_t m_Count = 0U;

public:
    ~MeshInfo() {
        m_Buffer.reset();
        m_BufferInfo = vk::DescriptorBufferInfo{VK_NULL_HANDLE, 0, VK_WHOLE_SIZE};
    }
};
