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

#include <Vulkan_Fence.hxx>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_Device.hxx>

#include <vulkan/vulkan.h>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_Fence, Vulkan_Object)

// =======================================================================
// function : Vulkan_Fence
// purpose  :
// =======================================================================
Vulkan_Fence::Vulkan_Fence()
: myVkFence (NULL)
{
  //
}

// =======================================================================
// function : ~Vulkan_Fence
// purpose  :
// =======================================================================
Vulkan_Fence::~Vulkan_Fence()
{
  releaseFence();
}

// =======================================================================
// function : releaseFence
// purpose  :
// =======================================================================
void Vulkan_Fence::releaseFence()
{
  if (myVkFence != NULL)
  {
    Vulkan_AssertOnRelease("Vulkan_Fence");
    vkDestroyFence (myDevice->Device(), myVkFence, myDevice->HostAllocator());
    myVkFence = NULL;
  }
  myDevice.Nullify();
}

// =======================================================================
// function : Create
// purpose  :
// =======================================================================
bool Vulkan_Fence::Create (const Handle(Vulkan_Device)& theDevice)
{
  if (myVkFence != NULL
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

  VkFenceCreateInfo aVkFenceInfo;
  aVkFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  aVkFenceInfo.pNext = NULL;
  aVkFenceInfo.flags = 0;

  VkResult aRes = vkCreateFence (theDevice->Device(), &aVkFenceInfo, theDevice->HostAllocator(), &myVkFence);
  if (aRes != VK_SUCCESS)
  {
    logFailureAndRelease ("failed to create fence", aRes);
    return false;
  }

  return true;
}

// =======================================================================
// function : Wait
// purpose  :
// =======================================================================
bool Vulkan_Fence::Wait()
{
  if (myVkFence == NULL)
  {
    return false;
  }

  VkResult aRes = vkWaitForFences (myDevice->Device(), 1, &myVkFence, VK_TRUE, UINT64_MAX);
  if (aRes != VK_SUCCESS)
  {
    logFailure ("failed to wait for fence", aRes);
    return false;
  }
  return true;
}

// =======================================================================
// function : Reset
// purpose  :
// =======================================================================
bool Vulkan_Fence::Reset()
{
  if (myVkFence == NULL)
  {
    return false;
  }

  VkResult aRes = vkResetFences (myDevice->Device(), 1, &myVkFence);
  if (aRes != VK_SUCCESS)
  {
    logFailure ("failed to reset fence", aRes);
    return false;
  }
  return true;
}
