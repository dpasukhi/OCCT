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

#include <Vulkan_RenderPass.hxx>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_Device.hxx>
#include <Vulkan_Image.hxx>
#include <Vulkan_Surface.hxx>

#include <vulkan/vulkan.h>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_RenderPass, Vulkan_Object)

// =======================================================================
// function : Vulkan_RenderPass
// purpose  :
// =======================================================================
Vulkan_RenderPass::Vulkan_RenderPass()
: myVkRenderPass (NULL)
{
  //
}

// =======================================================================
// function : ~Vulkan_RenderPass
// purpose  :
// =======================================================================
Vulkan_RenderPass::~Vulkan_RenderPass()
{
  releaseRenderPass();
}

// =======================================================================
// function : releaseRenderPass
// purpose  :
// =======================================================================
void Vulkan_RenderPass::releaseRenderPass()
{
  if (myVkRenderPass != NULL)
  {
    Vulkan_AssertOnRelease("Vulkan_RenderPass");
    vkDestroyRenderPass (myDevice->Device(), myVkRenderPass, myDevice->HostAllocator());
    myVkRenderPass = NULL;
  }
  myDevice.Nullify();
}

// =======================================================================
// function : Create
// purpose  :
// =======================================================================
bool Vulkan_RenderPass::Create (const Handle(Vulkan_Device)& theDevice,
                                const Handle(Vulkan_Surface)& theSurface)
{
  Release();
  if (theDevice.IsNull()
   || theDevice->Device() == NULL)
  {
    return false;
  }

  myDevice = theDevice;

  VkAttachmentDescription aVkAttachments[2] = {};
  {
    VkAttachmentDescription& aColorAttach = aVkAttachments[0];
    aColorAttach.flags          = 0;
    aColorAttach.format         = !theSurface.IsNull() ? theSurface->ColorFormat().format : VK_FORMAT_UNDEFINED;
    aColorAttach.samples        = VK_SAMPLE_COUNT_1_BIT;
    aColorAttach.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    aColorAttach.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    aColorAttach.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    aColorAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    aColorAttach.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    aColorAttach.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  }
  if (!theSurface->DepthImage().IsNull())
  {
    VkAttachmentDescription& aDepthAttach = aVkAttachments[1];
    aDepthAttach.flags          = 0;
    aDepthAttach.format         = theSurface->DepthImage()->SurfaceFormat().format;
    aDepthAttach.samples        = VK_SAMPLE_COUNT_1_BIT;
    aDepthAttach.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    aDepthAttach.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    aDepthAttach.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    aDepthAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    aDepthAttach.initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    aDepthAttach.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  }

  VkAttachmentReference aColorAttachRef = {};
  aColorAttachRef.attachment = 0;
  aColorAttachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference aDepthAttachRef = {};
  aDepthAttachRef.attachment = 1;
  aDepthAttachRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription aVkSubpassDesc;
  aVkSubpassDesc.flags = 0;
  aVkSubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  aVkSubpassDesc.inputAttachmentCount = 0;
  aVkSubpassDesc.pInputAttachments = NULL;
  aVkSubpassDesc.colorAttachmentCount = 1;
  aVkSubpassDesc.pColorAttachments = &aColorAttachRef;
  aVkSubpassDesc.pResolveAttachments = NULL;
  aVkSubpassDesc.pDepthStencilAttachment = &aDepthAttachRef;
  aVkSubpassDesc.preserveAttachmentCount = 0;
  aVkSubpassDesc.pPreserveAttachments = NULL;

  VkRenderPassCreateInfo aVkRenderPassInfo;
  aVkRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  aVkRenderPassInfo.pNext = NULL;
  aVkRenderPassInfo.flags = 0;
  aVkRenderPassInfo.attachmentCount = !theSurface->DepthImage().IsNull() ? 2 : 1;
  aVkRenderPassInfo.pAttachments = aVkAttachments;
  aVkRenderPassInfo.subpassCount = 1;
  aVkRenderPassInfo.pSubpasses = &aVkSubpassDesc;
  aVkRenderPassInfo.dependencyCount = 0;
  aVkRenderPassInfo.pDependencies = NULL;

  VkResult aRes = vkCreateRenderPass (theDevice->Device(), &aVkRenderPassInfo, theDevice->HostAllocator(), &myVkRenderPass);
  if (aRes != VK_SUCCESS)
  {
    logFailureAndRelease ("failed to create render pass", aRes);
    return false;
  }

  return true;
}
