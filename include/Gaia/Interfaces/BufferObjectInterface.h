/*
Copyright 2022-2023 Stephane Cuillerdier (aka aiekick)

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

#include <string>
#include <memory>
#include <Gaia/gaia.h>
#include <Gaia/Resources/VulkanRessource.h>
#include <Gaia/gaia.h>

class GAIA_API BufferObjectInterface {
protected:
	VulkanBufferObjectPtr m_BufferObjectPtr = nullptr;
	vk::DescriptorBufferInfo m_DescriptorBufferInfo = { VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };
	uint32_t m_BufferSize = 0U;

public:
	bool m_BufferObjectIsDirty = false;

public:
	virtual void UploadBufferObjectIfDirty(GaiApi::VulkanCoreWeak vVulkanCore) = 0;
	virtual bool CreateBufferObject(GaiApi::VulkanCoreWeak vVulkanCore) = 0;
	virtual void DestroyBufferObject()
	{
		m_BufferObjectPtr.reset();
		m_DescriptorBufferInfo = vk::DescriptorBufferInfo { VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };
	}

	//virtual std::string GetBufferObjectStructureHeader(const uint32_t& vBindingPoint) = 0;
	virtual vk::DescriptorBufferInfo* GetBufferInfo()
	{
		return &m_DescriptorBufferInfo;
	}

	// we reimplement it because vk::DescriptorBufferInfo.range 
	// can be not the full size becasue related to vk::DescriptorBufferInfo.offset
	virtual const uint32_t& GetBufferSIze()
	{
		return m_BufferSize;
	}
};
