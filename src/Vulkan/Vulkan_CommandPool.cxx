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

#include <Vulkan_CommandPool.hxx>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_CommandBuffer.hxx>
#include <Vulkan_Device.hxx>

#include <vulkan/vulkan.h>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_CommandPool, Vulkan_Object)

// =======================================================================
// function : Vulkan_CommandPool
// purpose  :
// =======================================================================
Vulkan_CommandPool::Vulkan_CommandPool()
: myVkCmdPool (NULL)
{
  //
}

// =======================================================================
// function : ~Vulkan_CommandPool
// purpose  :
// =======================================================================
Vulkan_CommandPool::~Vulkan_CommandPool()
{
  releasePool();
}

// =======================================================================
// function : releasePool
// purpose  :
// =======================================================================
void Vulkan_CommandPool::releasePool()
{
  if (myVkCmdPool != NULL)
  {
    Vulkan_AssertOnRelease("Vulkan_CommandPool");
    vkDestroyCommandPool (myDevice->Device(), myVkCmdPool, myDevice->HostAllocator());
    myVkCmdPool = NULL;
  }
  myDevice.Nullify();
}

// =======================================================================
// function : Create
// purpose  :
// =======================================================================
bool Vulkan_CommandPool::Create (const Handle(Vulkan_Device)& theDevice)
{
  if (myVkCmdPool != NULL
   && myDevice == theDevice)
  {
    return true;
  }

  Release();
  if (theDevice.IsNull()
   || theDevice->Device() == NULL)
  {
    return false;
  }

  myDevice = theDevice;

  VkCommandPoolCreateInfo aVkCmdPoolInfo;
  aVkCmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  aVkCmdPoolInfo.pNext = NULL;
  aVkCmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  aVkCmdPoolInfo.queueFamilyIndex = 0;

  VkResult aRes = vkCreateCommandPool (theDevice->Device(), &aVkCmdPoolInfo, theDevice->HostAllocator(), &myVkCmdPool);
  if (aRes != VK_SUCCESS)
  {
    logFailureAndRelease ("failed to create command pool", aRes);
    return false;
  }

  if (!ResetPool())
  {
    Release();
    return false;
  }

  return true;
}

// =======================================================================
// function : ResetPool
// purpose  :
// =======================================================================
bool Vulkan_CommandPool::ResetPool()
{
  if (myVkCmdPool == NULL)
  {
    return false;
  }

  VkResult aRes = vkResetCommandPool (myDevice->Device(), myVkCmdPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
  if (aRes != VK_SUCCESS)
  {
    logFailure ("failed to reset command pool", aRes);
    return false;
  }
  return true;
}

// =======================================================================
// function : AllocateBuffers
// purpose  :
// =======================================================================
Handle(Vulkan_CommandBuffer) Vulkan_CommandPool::AllocateBuffer()
{
  Handle(Vulkan_CommandBuffer) aBuffer = new Vulkan_CommandBuffer();
  return aBuffer->Create (this)
       ? aBuffer
       : Handle(Vulkan_CommandBuffer)();
}
