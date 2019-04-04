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

#include <Vulkan_CommandBuffer.hxx>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_CommandPool.hxx>
#include <Vulkan_Device.hxx>
#include <Vulkan_FrameBuffer.hxx>
#include <Vulkan_Pipeline.hxx>
#include <Vulkan_RenderPass.hxx>

#include <vulkan/vulkan.h>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_CommandBuffer, Vulkan_Object)

// =======================================================================
// function : Vulkan_CommandBuffer
// purpose  :
// =======================================================================
Vulkan_CommandBuffer::Vulkan_CommandBuffer()
: myVkCmdBuffer (NULL)
{
  //
}

// =======================================================================
// function : ~Vulkan_CommandBuffer
// purpose  :
// =======================================================================
Vulkan_CommandBuffer::~Vulkan_CommandBuffer()
{
  releaseBuffer();
}

// =======================================================================
// function : releaseBuffer
// purpose  :
// =======================================================================
void Vulkan_CommandBuffer::releaseBuffer()
{
  if (myVkCmdBuffer != NULL)
  {
    Vulkan_AssertOnRelease("Vulkan_CommandBuffer");
    vkFreeCommandBuffers (myDevice->Device(), myCmdPool->CommandPool(), 1, &myVkCmdBuffer);
    myVkCmdBuffer = NULL;
  }
  myCmdPool.Nullify();
  myDevice.Nullify();
}

// =======================================================================
// function : Create
// purpose  :
// =======================================================================
bool Vulkan_CommandBuffer::Create (const Handle(Vulkan_CommandPool)& thePool)
{
  if (thePool.IsNull()
   || thePool->CommandPool() == NULL)
  {
    Release();
    return false;
  }

  if (myVkCmdBuffer != NULL
   && myCmdPool == thePool)
  {
    return true;
  }

  Release();
  myDevice = thePool->Device();
  myCmdPool = thePool;

  VkCommandBufferAllocateInfo anAllocInfo;
  anAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  anAllocInfo.pNext = NULL;
  anAllocInfo.commandPool = myCmdPool->CommandPool();
  anAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  anAllocInfo.commandBufferCount = 1;

  VkResult aRes = vkAllocateCommandBuffers (myDevice->Device(), &anAllocInfo, &myVkCmdBuffer);
  if (aRes != VK_SUCCESS)
  {
    logFailureAndRelease ("failed to allocate command buffers", aRes);
    return false;
  }

  aRes = vkResetCommandBuffer (myVkCmdBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  if (aRes != VK_SUCCESS)
  {
    logFailureAndRelease ("failed to reset command buffer", aRes);
    return false;
  }

  return true;
}

// =======================================================================
// function : ResetCommandBuffer
// purpose  :
// =======================================================================
bool Vulkan_CommandBuffer::ResetCommandBuffer()
{
  if (myVkCmdBuffer == NULL)
  {
    return false;
  }

  VkResult aRes = vkResetCommandBuffer (myVkCmdBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  if (aRes != VK_SUCCESS)
  {
    logFailure ("failed to reset command buffer", aRes);
    return false;
  }
  return true;
}

// =======================================================================
// function : BeginCommandBuffer
// purpose  :
// =======================================================================
void Vulkan_CommandBuffer::BeginCommandBuffer (const Handle(Vulkan_RenderPass)& theRenderPass,
                                               const Handle(Vulkan_FrameBuffer)& theFrameBuffer)
{
  VkCommandBufferInheritanceInfo anInherInfo;
  anInherInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
  anInherInfo.pNext = NULL;
  anInherInfo.renderPass = theRenderPass->RenderPass();
  anInherInfo.subpass = 0;
  anInherInfo.framebuffer = theFrameBuffer->FrameBuffer();
  anInherInfo.occlusionQueryEnable = VK_FALSE;
  anInherInfo.queryFlags = 0;
  anInherInfo.pipelineStatistics = 0;

  VkCommandBufferBeginInfo aBuffBeginInfo;
  aBuffBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  aBuffBeginInfo.pNext = NULL;
  aBuffBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  aBuffBeginInfo.pInheritanceInfo = &anInherInfo;

  vkBeginCommandBuffer (myVkCmdBuffer, &aBuffBeginInfo);
}

// =======================================================================
// function : EndCommandBuffer
// purpose  :
// =======================================================================
void Vulkan_CommandBuffer::EndCommandBuffer()
{
  if (myVkCmdBuffer != NULL)
  {
    vkEndCommandBuffer (myVkCmdBuffer);
  }
}

// =======================================================================
// function : BeginRenderPass
// purpose  :
// =======================================================================
void Vulkan_CommandBuffer::BeginRenderPass (const Handle(Vulkan_RenderPass)& theRenderPass,
                                            const Handle(Vulkan_FrameBuffer)& theFrameBuffer,
                                            const Graphic3d_Vec2u& theSize,
                                            const Graphic3d_Vec4* theClearColor)
{
  if (myVkCmdBuffer == NULL)
  {
    return;
  }

  uint32_t aNbClears = 0;
  VkClearValue aClearValues[2];
  if (theClearColor != NULL)
  {
    aClearValues[aNbClears].color.float32[0] = theClearColor->r();
    aClearValues[aNbClears].color.float32[1] = theClearColor->g();
    aClearValues[aNbClears].color.float32[2] = theClearColor->b();
    aClearValues[aNbClears].color.float32[3] = theClearColor->a();
    ++aNbClears;
  }
  {
    aClearValues[aNbClears].depthStencil.depth   = 1.0f;
    aClearValues[aNbClears].depthStencil.stencil = 0;
    ++aNbClears;
  }

  VkRenderPassBeginInfo aPassBeginInfo;
  aPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  aPassBeginInfo.pNext = NULL;
  aPassBeginInfo.renderPass  = theRenderPass->RenderPass();
  aPassBeginInfo.framebuffer = theFrameBuffer->FrameBuffer();
  aPassBeginInfo.renderArea = VkRect2D{
    VkOffset2D{0, 0},
    VkExtent2D{theSize.x(), theSize.y()}
  };
  aPassBeginInfo.clearValueCount = aNbClears;
  aPassBeginInfo.pClearValues = aClearValues;

  vkCmdBeginRenderPass (myVkCmdBuffer, &aPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

// =======================================================================
// function : EndRenderPass
// purpose  :
// =======================================================================
void Vulkan_CommandBuffer::EndRenderPass()
{
  if (myVkCmdBuffer != NULL)
  {
    vkCmdEndRenderPass (myVkCmdBuffer);
  }
}

// =======================================================================
// function : BindPipeline
// purpose  :
// =======================================================================
void Vulkan_CommandBuffer::BindPipeline (const Handle(Vulkan_Pipeline)& thePipeline)
{
  if (myVkCmdBuffer != NULL)
  {
    vkCmdBindPipeline (myVkCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, thePipeline->Pipeline());
  }
}
