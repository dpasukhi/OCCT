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

#include <Vulkan_FrameBuffer.hxx>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_Device.hxx>
#include <Vulkan_Image.hxx>
#include <Vulkan_RenderPass.hxx>
#include <Vulkan_Surface.hxx>

#include <vulkan/vulkan.h>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_FrameBuffer, Vulkan_Object)

// =======================================================================
// function : Vulkan_FrameBuffer
// purpose  :
// =======================================================================
Vulkan_FrameBuffer::Vulkan_FrameBuffer()
: myVkFramebuffer (NULL),
  myVkImageView (NULL)
{
  //
}

// =======================================================================
// function : ~Vulkan_FrameBuffer
// purpose  :
// =======================================================================
Vulkan_FrameBuffer::~Vulkan_FrameBuffer()
{
  releaseBuffer();
}

// =======================================================================
// function : releaseBuffer
// purpose  :
// =======================================================================
void Vulkan_FrameBuffer::releaseBuffer()
{
  if (myVkFramebuffer != NULL)
  {
    Vulkan_AssertOnRelease("Vulkan_FrameBuffer");
    vkDestroyFramebuffer (myDevice->Device(), myVkFramebuffer, myDevice->HostAllocator());
    myVkFramebuffer = NULL;
  }
  myRenderPass.Nullify();
  myDepth.Nullify();
  mySurface.Nullify();
  myDevice.Nullify();
  myVkImageView = NULL;
}

// =======================================================================
// function : Create
// purpose  :
// =======================================================================
bool Vulkan_FrameBuffer::Create (const Handle(Vulkan_RenderPass)& theRenderPass,
                                 const Handle(Vulkan_Surface)& theSurface,
                                 const uint32_t theChainIndex)
{
  if (theRenderPass.IsNull()
   || theSurface.IsNull())
  {
    Release();
    return false;
  }

  const VkImageView anImageView = theSurface->ImageViews()[theChainIndex];
  if (myRenderPass == theRenderPass
   && mySurface == theSurface
   && myVkImageView == anImageView
   && myDepth == theSurface->DepthImage()
   && mySize == theSurface->Size())
  {
    return true;
  }

  Release();
  myRenderPass = theRenderPass;
  mySurface = theSurface;
  myDepth   = theSurface->DepthImage();
  myDevice  = theRenderPass->Device();
  myVkImageView = anImageView;
  mySize = theSurface->Size();

  VkFramebufferCreateInfo aVkFboInfo = {};
  const VkImageView aVkImageViews[2] =
  {
    anImageView,
    !myDepth.IsNull() ? myDepth->ImageView() : NULL,
  };
  aVkFboInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  aVkFboInfo.pNext = NULL;
  aVkFboInfo.flags = 0;
  aVkFboInfo.renderPass = theRenderPass->RenderPass();
  aVkFboInfo.attachmentCount = 2;
  aVkFboInfo.pAttachments = aVkImageViews;
  aVkFboInfo.width  = theSurface->Size().x();
  aVkFboInfo.height = theSurface->Size().y();
  aVkFboInfo.layers = 1;

  VkResult aRes = vkCreateFramebuffer (myDevice->Device(), &aVkFboInfo, myDevice->HostAllocator(), &myVkFramebuffer);
  if (aRes != VK_SUCCESS)
  {
    logFailureAndRelease ("failed to create framebuffer", aRes);
    return false;
  }

  return true;
}
