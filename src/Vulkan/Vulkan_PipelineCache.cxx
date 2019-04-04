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

#include <Vulkan_PipelineCache.hxx>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_Device.hxx>

#include <vulkan/vulkan.h>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_PipelineCache, Vulkan_Object)

// =======================================================================
// function : Vulkan_PipelineCache
// purpose  :
// =======================================================================
Vulkan_PipelineCache::Vulkan_PipelineCache()
: myVkPipelineCache (NULL)
{
  //
}

// =======================================================================
// function : ~Vulkan_PipelineCache
// purpose  :
// =======================================================================
Vulkan_PipelineCache::~Vulkan_PipelineCache()
{
  releasePipelineCache();
}

// =======================================================================
// function : releasePipelineCache
// purpose  :
// =======================================================================
void Vulkan_PipelineCache::releasePipelineCache()
{
  if (myVkPipelineCache != NULL)
  {
    Vulkan_AssertOnRelease("Vulkan_PipelineCache");
    vkDestroyPipelineCache (myDevice->Device(), myVkPipelineCache, myDevice->HostAllocator());
    myVkPipelineCache = NULL;
  }
  myDevice.Nullify();
}

// =======================================================================
// function : Create
// purpose  :
// =======================================================================
bool Vulkan_PipelineCache::Create (const Handle(Vulkan_Device)& theDevice)
{
  if (myVkPipelineCache != NULL
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

  VkPipelineCacheCreateInfo aVkPipelineCacheInfo;
  aVkPipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  aVkPipelineCacheInfo.pNext = NULL;
  aVkPipelineCacheInfo.flags = 0;
  aVkPipelineCacheInfo.initialDataSize = 0;
  aVkPipelineCacheInfo.pInitialData = NULL;

  VkResult aRes = vkCreatePipelineCache (theDevice->Device(), &aVkPipelineCacheInfo, theDevice->HostAllocator(), &myVkPipelineCache);
  if (aRes != VK_SUCCESS)
  {
    logFailureAndRelease ("failed to create pipeline cache", aRes);
    return false;
  }
  return true;
}
