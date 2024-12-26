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

#include <map>
#include <array>
#include <memory>
#include <Gaia/gaia.h>
#include <ImWidgets.h>
#include <Gaia/Core/VulkanCore.h>

template<size_t size_of_array>
class StorageBufferInputInterface
{
protected:
	std::array<vk::DescriptorBufferInfo, size_of_array> m_StorageBuffers;
	std::array<uint32_t, size_of_array> m_StorageBufferSize;

public:
	virtual void SetStorageBuffer(const uint32_t& vBindingPoint, vk::DescriptorBufferInfo* vStorageBuffer, uint32_t* vStorageBufferSize) = 0;
};