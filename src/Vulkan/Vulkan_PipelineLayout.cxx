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

#include <Vulkan_PipelineLayout.hxx>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_DescriptorSetLayout.hxx>
#include <Vulkan_Device.hxx>

#include <vulkan/vulkan.h>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_PipelineLayout, Vulkan_Object)

// =======================================================================
// function : Vulkan_PipelineLayout
// purpose  :
// =======================================================================
Vulkan_PipelineLayout::Vulkan_PipelineLayout()
: myVkPipelineLayout (NULL)
{
  //
}

// =======================================================================
// function : ~Vulkan_PipelineLayout
// purpose  :
// =======================================================================
Vulkan_PipelineLayout::~Vulkan_PipelineLayout()
{
  releasePipelineLayout();
}

// =======================================================================
// function : releasePipelineLayout
// purpose  :
// =======================================================================
void Vulkan_PipelineLayout::releasePipelineLayout()
{
  if (myVkPipelineLayout != NULL)
  {
    Vulkan_AssertOnRelease("Vulkan_PipelineLayout");
    vkDestroyPipelineLayout (myDevice->Device(), myVkPipelineLayout, myDevice->HostAllocator());
    myVkPipelineLayout = NULL;
  }
  myDescSetLayout.Nullify();
  myDevice.Nullify();
}

// =======================================================================
// function : Create
// purpose  :
// =======================================================================
bool Vulkan_PipelineLayout::Create (const Handle(Vulkan_Device)& theDevice,
                                    const Handle(Vulkan_DescriptorSetLayout)& theDescSetLayout)
{
  if (myVkPipelineLayout != NULL
   && myDevice == theDevice
   && myDescSetLayout == theDescSetLayout)
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
  myDescSetLayout = theDescSetLayout;

  VkDescriptorSetLayout aDescLayouts[2] = { theDescSetLayout->DescriptorSetLayout(), theDescSetLayout->DescriptorSetLayout() };

  VkPipelineLayoutCreateInfo aVkPipelineLayoutInfo;
  aVkPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  aVkPipelineLayoutInfo.pNext = NULL;
  aVkPipelineLayoutInfo.flags = 0;
  aVkPipelineLayoutInfo.setLayoutCount = 2;
  aVkPipelineLayoutInfo.pSetLayouts = aDescLayouts;
  aVkPipelineLayoutInfo.pushConstantRangeCount = 0;
  aVkPipelineLayoutInfo.pPushConstantRanges = NULL;

  VkResult aRes = vkCreatePipelineLayout (theDevice->Device(), &aVkPipelineLayoutInfo, theDevice->HostAllocator(), &myVkPipelineLayout);
  if (aRes != VK_SUCCESS)
  {
    logFailureAndRelease ("failed to create pipeline layout", aRes);
    return false;
  }

  return true;
}
