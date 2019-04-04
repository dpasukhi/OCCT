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

#include <Vulkan_DescriptorPool.hxx>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_Device.hxx>

#include <vulkan/vulkan.h>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_DescriptorPool, Vulkan_Object)

// =======================================================================
// function : Vulkan_DescriptorPool
// purpose  :
// =======================================================================
Vulkan_DescriptorPool::Vulkan_DescriptorPool()
: myVkDescPool (NULL)
{
  //
}

// =======================================================================
// function : ~Vulkan_DescriptorPool
// purpose  :
// =======================================================================
Vulkan_DescriptorPool::~Vulkan_DescriptorPool()
{
  releasePool();
}

// =======================================================================
// function : releaseFence
// purpose  :
// =======================================================================
void Vulkan_DescriptorPool::releasePool()
{
  if (myVkDescPool != NULL)
  {
    Vulkan_AssertOnRelease("Vulkan_DescriptorPool");
    vkDestroyDescriptorPool (myDevice->Device(), myVkDescPool, myDevice->HostAllocator());
    myVkDescPool = NULL;
  }
  myDevice.Nullify();
}

// =======================================================================
// function : Create
// purpose  :
// =======================================================================
bool Vulkan_DescriptorPool::Create (const Handle(Vulkan_Device)& theDevice)
{
  Release();
  if (theDevice.IsNull()
   || theDevice->Device() == NULL)
  {
    return false;
  }

  myDevice = theDevice;

  VkDescriptorPoolSize aVkPoolSize = {};
  aVkPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  aVkPoolSize.descriptorCount = 10;

  VkDescriptorPoolCreateInfo aVkPoolInfo = {};
  aVkPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  aVkPoolInfo.poolSizeCount = 1;
  aVkPoolInfo.pPoolSizes = &aVkPoolSize;
  aVkPoolInfo.maxSets = 10;
  VkResult aRes = vkCreateDescriptorPool (theDevice->Device(), &aVkPoolInfo, theDevice->HostAllocator(), &myVkDescPool);
  if (aRes != VK_SUCCESS)
  {
    logFailureAndRelease ("failed to create descriptor pool", aRes);
    return false;
  }

  return true;
}

// =======================================================================
// function : ResetPool
// purpose  :
// =======================================================================
bool Vulkan_DescriptorPool::ResetPool()
{
  if (myVkDescPool == NULL)
  {
    return false;
  }

  VkResult aRes = vkResetDescriptorPool (myDevice->Device(), myVkDescPool, 0);
  if (aRes != VK_SUCCESS)
  {
    logFailure ("failed to reset descriptor pool", aRes);
    return false;
  }
  return true;
}
