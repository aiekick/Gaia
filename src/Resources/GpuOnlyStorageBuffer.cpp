#include <Gaia/Resources/GpuOnlyStorageBuffer.h>

GpuOnlyStorageBufferPtr GpuOnlyStorageBuffer::Create(GaiApi::VulkanCoreWeak vVulkanCore) {
    return std::make_shared<GpuOnlyStorageBuffer>(vVulkanCore);
}

GpuOnlyStorageBuffer::GpuOnlyStorageBuffer(GaiApi::VulkanCoreWeak vVulkanCore) : m_VulkanCore(vVulkanCore) {
}

GpuOnlyStorageBuffer::~GpuOnlyStorageBuffer() {
    DestroyBuffer();
}

bool GpuOnlyStorageBuffer::CreateBuffer(const uint32_t& vDatasSizeInBytes,
    const uint32_t& vDatasCount,
    const VmaMemoryUsage& vVmaMemoryUsage,
    const vk::BufferUsageFlags& vBufferUsageFlags) {
    if (vDatasCount && vDatasSizeInBytes && vDatasCount) {
        DestroyBuffer();

        auto sizeInBytes = vDatasCount * vDatasSizeInBytes;

        // gpu only since no udpate will be done on cpu side

        vk::BufferCreateInfo storageBufferInfo = {};
        VmaAllocationCreateInfo storageAllocInfo = {};
        storageBufferInfo.size = sizeInBytes;
        storageBufferInfo.sharingMode = vk::SharingMode::eExclusive;
        storageAllocInfo.usage = vVmaMemoryUsage;

        if (vVmaMemoryUsage == VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_TO_CPU) {
            storageBufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc | vBufferUsageFlags;
            m_BufferObjectPtr =
                GaiApi::VulkanRessource::createSharedBufferObject(m_VulkanCore, storageBufferInfo, storageAllocInfo, "GpuOnlyStorageBuffer");
        } else if (vVmaMemoryUsage == VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY) {
            storageBufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer | vBufferUsageFlags;
            m_BufferObjectPtr =
                GaiApi::VulkanRessource::createSharedBufferObject(m_VulkanCore, storageBufferInfo, storageAllocInfo, "GpuOnlyStorageBuffer");
        }

        if (m_BufferObjectPtr && m_BufferObjectPtr->buffer) {
            m_BufferSize = vDatasCount;

            m_DescriptorBufferInfo.buffer = m_BufferObjectPtr->buffer;
            m_DescriptorBufferInfo.offset = 0U;
            m_DescriptorBufferInfo.range = sizeInBytes;

            return true;
        }
    }

    return false;
}

VulkanBufferObjectPtr GpuOnlyStorageBuffer::GetBufferObjectPtr() {
    return m_BufferObjectPtr;
}

vk::DescriptorBufferInfo* GpuOnlyStorageBuffer::GetBufferInfo() {
    return &m_DescriptorBufferInfo;
}

const uint32_t& GpuOnlyStorageBuffer::GetBufferSize() {
    return m_BufferSize;
}

vk::Buffer* GpuOnlyStorageBuffer::GetVulkanBuffer() {
    if (m_BufferObjectPtr) {
        return &m_BufferObjectPtr->buffer;
    }

    return nullptr;
}

void GpuOnlyStorageBuffer::DestroyBuffer() {
    m_BufferObjectPtr.reset();
    m_DescriptorBufferInfo = vk::DescriptorBufferInfo{VK_NULL_HANDLE, 0, VK_WHOLE_SIZE};
    m_BufferSize = 0U;
}

bool GpuOnlyStorageBuffer::MapMemory(void* vMappedMemory) {
    if (m_BufferObjectPtr) {
        return vmaMapMemory(GaiApi::VulkanCore::sAllocator, m_BufferObjectPtr->alloc_meta, &vMappedMemory) == VK_SUCCESS;
    }
    return false;
}

void GpuOnlyStorageBuffer::UnmapMemory() {
    if (m_BufferObjectPtr) {
        vmaUnmapMemory(GaiApi::VulkanCore::sAllocator, m_BufferObjectPtr->alloc_meta);
    }
}
