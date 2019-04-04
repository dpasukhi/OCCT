// Created by: Kirill GAVRILOV
// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.

#include <Vulkan_DeviceMemoryAllocator.hxx>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_Device.hxx>
#include <Vulkan_DeviceMemory.hxx>

#include <vulkan/vulkan.h>

#ifdef max
  #undef min
  #undef max
#endif
#define VMA_IMPLEMENTATION
#include <Standard_WarningsDisable.hxx>
#include <vk_mem_alloc.h>
#include <Standard_WarningsRestore.hxx>

namespace
{
  static VmaMemoryUsage memUsageOcctToVma (Vulkan_DeviceMemoryUsage theUsage)
  {
    switch (theUsage)
    {
      case Vulkan_DeviceMemoryUsage_UNKNOWN:  return VMA_MEMORY_USAGE_UNKNOWN;
      case Vulkan_DeviceMemoryUsage_GpuOnly:  return VMA_MEMORY_USAGE_GPU_ONLY;
      case Vulkan_DeviceMemoryUsage_CpuOnly:  return VMA_MEMORY_USAGE_CPU_ONLY;
      case Vulkan_DeviceMemoryUsage_CpuToGpu: return VMA_MEMORY_USAGE_CPU_TO_GPU;
      case Vulkan_DeviceMemoryUsage_GpuToCpu: return VMA_MEMORY_USAGE_GPU_TO_CPU;
    }
    return VMA_MEMORY_USAGE_UNKNOWN;
  }
}

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_DeviceMemoryAllocator, Standard_Transient)

// =======================================================================
// function : Vulkan_DeviceMemory
// purpose  :
// =======================================================================
Vulkan_DeviceMemoryAllocator::Vulkan_DeviceMemoryAllocator()
: myVmaAllocator (NULL),
  myDevice (NULL)
{
  //
}

// =======================================================================
// function : ~Vulkan_DeviceMemoryAllocator
// purpose  :
// =======================================================================
Vulkan_DeviceMemoryAllocator::~Vulkan_DeviceMemoryAllocator()
{
  releaseAllocator();
}

// =======================================================================
// function : releaseAllocator
// purpose  :
// =======================================================================
void Vulkan_DeviceMemoryAllocator::releaseAllocator()
{
  if (myVmaAllocator != NULL)
  {
    vmaDestroyAllocator (myVmaAllocator);
    myVmaAllocator = NULL;
  }
  myDevice = NULL;
}

// =======================================================================
// function : Create
// purpose  :
// =======================================================================
bool Vulkan_DeviceMemoryAllocator::Create (const Handle(Vulkan_Device)& theDevice,
                                           bool toUseDedicatedAllocs)
{
  release();
  if (theDevice.IsNull()
   || theDevice->Device() == NULL)
  {
    return false;
  }

  myDevice = theDevice.get();

  VmaAllocatorCreateInfo aVmaAllocInfo = {};
  aVmaAllocInfo.physicalDevice = theDevice->PhysicalDevice();
  aVmaAllocInfo.device = theDevice->Device();
  aVmaAllocInfo.flags  = 0;
  if (toUseDedicatedAllocs)
  {
    // will be used by vmaCreateBuffer() and vmaCreateImage()
    aVmaAllocInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
  }

  VkResult aRes = vmaCreateAllocator (&aVmaAllocInfo, &myVmaAllocator);
  if (aRes != VK_SUCCESS)
  {
    Message::DefaultMessenger()->Send (TCollection_AsciiString("Vulkan_DeviceMemoryAllocator, unable to create memory allocator: ") + Vulkan_Device::FormatVkError (aRes));
    release();
    return false;
  }

  return true;
}

// =======================================================================
// function : memoryRegionInfo
// purpose  :
// =======================================================================
Vulkan_DeviceMemoryInfo Vulkan_DeviceMemoryAllocator::memoryRegionInfo (const Vulkan_DeviceMemory& theMem) const
{
  Vulkan_DeviceMemoryInfo anInfo;
  if (myVmaAllocator != NULL)
  {
    VmaAllocationInfo anAllocInfo = {};
    vmaGetAllocationInfo (myVmaAllocator, (VmaAllocation )theMem.myOpaque, &anAllocInfo);

    anInfo.DeviceMemory = anAllocInfo.deviceMemory;
    anInfo.Offset = anAllocInfo.offset;
    anInfo.Size   = anAllocInfo.size;
  }
  else
  {
    anInfo.DeviceMemory = (VkDeviceMemory )theMem.myOpaque;
    anInfo.Offset = 0;
    anInfo.Size   = 0;
  }
  return anInfo;
}

// =======================================================================
// function : memoryRegionFree
// purpose  :
// =======================================================================
void Vulkan_DeviceMemoryAllocator::memoryRegionFree (Vulkan_DeviceMemory& theMem)
{
  if (theMem.myOpaque != NULL)
  {
    if (myVmaAllocator != NULL)
    {
      vmaFreeMemory (myVmaAllocator, (VmaAllocation )theMem.myOpaque);
    }
    else
    {
      vkFreeMemory (myDevice->Device(), (VkDeviceMemory )theMem.myOpaque, myDevice->HostAllocator());
    }
    theMem.myOpaque = NULL;
  }
}

// =======================================================================
// function : Allocate
// purpose  :
// =======================================================================
Handle(Vulkan_DeviceMemory) Vulkan_DeviceMemoryAllocator::Allocate (const VkMemoryRequirements& theReqs,
                                                                    Vulkan_DeviceMemoryUsage theUsage)
{
  if (myVmaAllocator == NULL)
  {
    VkDeviceMemory aVkDevMem = myDevice->allocateDeviceMemory (theReqs);
    if (aVkDevMem == NULL)
    {
      return Handle(Vulkan_DeviceMemory)();
    }
    Handle(Vulkan_DeviceMemory) aDevMem = new Vulkan_DeviceMemory (NULL, aVkDevMem);
    return aDevMem;
  }

  VmaAllocation aVmaAlloc = NULL;
  VmaAllocationCreateInfo anAllocCreateInfo = {};
  anAllocCreateInfo.usage = memUsageOcctToVma (theUsage);

  VmaAllocationInfo anAllocInfo = {};
  VkResult aRes = vmaAllocateMemory (myVmaAllocator, &theReqs, &anAllocCreateInfo, &aVmaAlloc, &anAllocInfo);
  if (aRes != VK_SUCCESS)
  {
    Message::DefaultMessenger()->Send (TCollection_AsciiString ("Vulkan_DeviceMemoryAllocator, failed to allocate device memory [")
                                       + int(theReqs.size) + "]:" + Vulkan_Device::FormatVkError (aRes), Message_Fail);
    return Handle(Vulkan_DeviceMemory)();
  }

  Handle(Vulkan_DeviceMemory) aDevMem = new Vulkan_DeviceMemory (this, aVmaAlloc);
  return aDevMem;
}
