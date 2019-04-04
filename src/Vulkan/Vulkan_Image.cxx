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

#include <Vulkan_Image.hxx>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_Device.hxx>
#include <Vulkan_DeviceMemory.hxx>
#include <Vulkan_DeviceMemoryAllocator.hxx>

#include <vulkan/vulkan.h>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_Image, Vulkan_Object)

// =======================================================================
// function : Vulkan_Image
// purpose  :
// =======================================================================
Vulkan_Image::Vulkan_Image()
: myVkFormat (new VkSurfaceFormatKHR()),
  myVkImage (NULL)
{
  myVkFormat->format = VK_FORMAT_UNDEFINED;
  myVkFormat->colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
}

// =======================================================================
// function : ~Vulkan_Image
// purpose  :
// =======================================================================
Vulkan_Image::~Vulkan_Image()
{
  releaseImage();
}

// =======================================================================
// function : releaseImage
// purpose  :
// =======================================================================
void Vulkan_Image::releaseImage()
{
  if (myVkImageView != NULL)
  {
    Vulkan_AssertOnRelease("Vulkan_Image");
    vkDestroyImageView (myDevice->Device(), myVkImageView, myDevice->HostAllocator());
    myVkImageView = NULL;
  }
  if (myVkImage != NULL)
  {
    Vulkan_AssertOnRelease("Vulkan_Image");
    vkDestroyImage (myDevice->Device(), myVkImage, myDevice->HostAllocator());
    myVkImage = NULL;
  }
  myDepthMemory.Nullify();
  myDevice.Nullify();
}

// =======================================================================
// function : CreateDepthStencil
// purpose  :
// =======================================================================
bool Vulkan_Image::CreateDepthStencil (const Handle(Vulkan_Device)& theDevice,
                                       const Graphic3d_Vec2u& theSize)
{
  Release();
  if (theDevice.IsNull()
   || theDevice->Device() == NULL
   || theSize.x() == 0
   || theSize.y() == 0)
  {
    return false;
  }

  myDevice = theDevice;
  mySize = theSize;
  myVkFormat->format = VK_FORMAT_D32_SFLOAT_S8_UINT; ///VK_FORMAT_D24_UNORM_S8_UINT;///VK_FORMAT_D24_UNORM_S8_UINT;
  //myVkFormat->format = VK_FORMAT_D16_UNORM;

  VkImageCreateInfo aDepthInfo = {};
  aDepthInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  aDepthInfo.imageType   = VK_IMAGE_TYPE_2D;
  aDepthInfo.format      = myVkFormat->format;
  aDepthInfo.extent      = { theSize.x(), theSize.y(), 1 };
  aDepthInfo.mipLevels   = 1;
  aDepthInfo.arrayLayers = 1;
  aDepthInfo.samples     = VK_SAMPLE_COUNT_1_BIT;
  aDepthInfo.tiling      = VK_IMAGE_TILING_OPTIMAL;
  aDepthInfo.usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  aDepthInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  aDepthInfo.queueFamilyIndexCount = 0;
  aDepthInfo.pQueueFamilyIndices = NULL;
  ///aDepthInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; /// TODO why???
  aDepthInfo.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkResult aRes = vkCreateImage (theDevice->Device(), &aDepthInfo, theDevice->HostAllocator(), &myVkImage);
  if (aRes != VK_SUCCESS)
  {
    logFailureAndRelease ("failed to create depth image", aRes);
    return false;
  }

  VkMemoryRequirements aMemReqs = {};
  vkGetImageMemoryRequirements (theDevice->Device(), myVkImage, &aMemReqs);
  myDepthMemory = theDevice->DeviceMemoryAllocator()->Allocate (aMemReqs, Vulkan_DeviceMemoryUsage_GpuOnly);
  if (myDepthMemory.IsNull())
  {
    logFailureAndRelease ("failed allocating depth image memory", aRes);
    return false;
  }

  const Vulkan_DeviceMemoryInfo aDevMemInfo = myDepthMemory->DeviceMemoryInfo();
  aRes = vkBindImageMemory (theDevice->Device(), myVkImage, aDevMemInfo.DeviceMemory, aDevMemInfo.Offset);
  if (aRes != VK_SUCCESS)
  {
    logFailureAndRelease ("failed to bind image memory", aRes);
    return false;
  }

  VkImageViewCreateInfo aDepthImgViewInfo = {};
  aDepthImgViewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  aDepthImgViewInfo.image    = myVkImage;
  aDepthImgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  aDepthImgViewInfo.format   = aDepthInfo.format;
  aDepthImgViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  aDepthImgViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  aDepthImgViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  aDepthImgViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  aDepthImgViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  aDepthImgViewInfo.subresourceRange.baseMipLevel   = 0;
  aDepthImgViewInfo.subresourceRange.levelCount     = 1;
  aDepthImgViewInfo.subresourceRange.baseArrayLayer = 0;
  aDepthImgViewInfo.subresourceRange.layerCount     = 1;

  aRes = vkCreateImageView (theDevice->Device(), &aDepthImgViewInfo, theDevice->HostAllocator(), &myVkImageView);
  if (aRes != VK_SUCCESS)
  {
    logFailureAndRelease ("failed to create image view", aRes);
    return false;
  }

  return true;
}
