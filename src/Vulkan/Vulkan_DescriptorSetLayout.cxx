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

#include <Vulkan_DescriptorSetLayout.hxx>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_Device.hxx>

#include <vulkan/vulkan.h>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_DescriptorSetLayout, Vulkan_Object)

// =======================================================================
// function : Vulkan_DescriptorSetLayout
// purpose  :
// =======================================================================
Vulkan_DescriptorSetLayout::Vulkan_DescriptorSetLayout()
: myVkDescSetLayout (NULL)
{
  //
}

// =======================================================================
// function : ~Vulkan_DescriptorSetLayout
// purpose  :
// =======================================================================
Vulkan_DescriptorSetLayout::~Vulkan_DescriptorSetLayout()
{
  releaseLayout();
}

// =======================================================================
// function : releaseLayout
// purpose  :
// =======================================================================
void Vulkan_DescriptorSetLayout::releaseLayout()
{
  if (myVkDescSetLayout != NULL)
  {
    Vulkan_AssertOnRelease("Vulkan_DescriptorSetLayout");
    vkDestroyDescriptorSetLayout (myDevice->Device(), myVkDescSetLayout, myDevice->HostAllocator());
    myVkDescSetLayout = NULL;
  }
  myDevice.Nullify();
}

// =======================================================================
// function : Create
// purpose  :
// =======================================================================
bool Vulkan_DescriptorSetLayout::Create (const Handle(Vulkan_Device)& theDevice)
{
  Release();
  if (theDevice.IsNull()
   || theDevice->Device() == NULL)
  {
    return false;
  }

  myDevice = theDevice;

  VkDescriptorSetLayoutBinding aBindings[1] = {};
  {
    VkDescriptorSetLayoutBinding& aBinding = aBindings[0];
    aBinding.binding = 0;
    aBinding.descriptorCount = 1;
    aBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    aBinding.pImmutableSamplers = NULL;
    aBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    ///aBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  }
  /*{
    VkDescriptorSetLayoutBinding& aBinding = aBindings[1];
    aBinding.binding = 1;
    aBinding.descriptorCount = 1;
    aBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    aBinding.pImmutableSamplers = NULL;
    aBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    ///aBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  }*/

  VkDescriptorSetLayoutCreateInfo aVkDescLayoutInfo = {};
  aVkDescLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  aVkDescLayoutInfo.bindingCount = 1;
  aVkDescLayoutInfo.pBindings = aBindings;
  VkResult aRes = vkCreateDescriptorSetLayout (theDevice->Device(), &aVkDescLayoutInfo, theDevice->HostAllocator(), &myVkDescSetLayout);
  if (aRes != VK_SUCCESS)
  {
    logFailureAndRelease ("failed to create descriptor set layout", aRes);
    return false;
  }

  return true;
}
