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

#if defined(_WIN32)
  #include <windows.h>

  #define VK_USE_PLATFORM_WIN32_KHR 1
#endif

#include <Vulkan_Surface.hxx>

#include <Aspect_Window.hxx>
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_Device.hxx>
#include <Vulkan_Fence.hxx>
#include <Vulkan_Image.hxx>

#include <vulkan/vulkan.h>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_Surface, Vulkan_Object)

// =======================================================================
// function : Vulkan_Surface
// purpose  :
// =======================================================================
Vulkan_Surface::Vulkan_Surface()
: mySwapFence (new Vulkan_Fence()),
  myDepthImage (new Vulkan_Image()),
  myVkSurface (NULL),
  myVkSwapChain (NULL),
  myVkFormat (new VkSurfaceFormatKHR())
{
  myVkFormat->format = VK_FORMAT_UNDEFINED;
  myVkFormat->colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
}

// =======================================================================
// function : ~Vulkan_Surface
// purpose  :
// =======================================================================
Vulkan_Surface::~Vulkan_Surface()
{
  releaseSurface();
}

// =======================================================================
// function : releaseSurface
// purpose  :
// =======================================================================
void Vulkan_Surface::releaseSurface()
{
  mySwapFence->Release();
  myDepthImage->Release();
  for (size_t anImgIter = 0; anImgIter < myVkImageViews.size(); ++anImgIter)
  {
    VkImageView& aVkImageView = myVkImageViews[anImgIter];
    if (aVkImageView != NULL)
    {
      Vulkan_AssertOnRelease("Vulkan_Surface");
      vkDestroyImageView (myDevice->Device(), aVkImageView, myDevice->HostAllocator());
      aVkImageView = NULL;
    }
  }
  myVkImages.clear();
  myVkImageViews.clear();
  if (myVkSwapChain != NULL)
  {
    Vulkan_AssertOnRelease("Vulkan_Surface");
    vkDestroySwapchainKHR (myDevice->Device(), myVkSwapChain, myDevice->HostAllocator());
    myVkSwapChain = NULL;
  }
  if (myVkSurface != NULL)
  {
    Vulkan_AssertOnRelease("Vulkan_Surface");
    vkDestroySurfaceKHR (myDevice->Instance(), myVkSurface, myDevice->HostAllocator());
    myVkSurface = NULL;
  }
  myDevice.Nullify();
}

// =======================================================================
// function : Init
// purpose  :
// =======================================================================
bool Vulkan_Surface::Init (const Handle(Vulkan_Device)& theDevice,
                           const Handle(Aspect_Window)& theWindow)
{
  Release();
  if (theDevice.IsNull()
   || theDevice->Device() == NULL
   || theWindow.IsNull())
  {
    return false;
  }

  if (!mySwapFence->Create (theDevice))
  {
    return false;
  }

  myDevice = theDevice;

#if defined(_WIN32)
  VkWin32SurfaceCreateInfoKHR aVkWin32SurfInfo;
  aVkWin32SurfInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  aVkWin32SurfInfo.pNext = NULL;
  aVkWin32SurfInfo.flags = 0;
  aVkWin32SurfInfo.hinstance = GetModuleHandleW (NULL);
  aVkWin32SurfInfo.hwnd = (HWND )theWindow->NativeHandle();

  VkResult aRes = vkCreateWin32SurfaceKHR (theDevice->Instance(), &aVkWin32SurfInfo, theDevice->HostAllocator(), &myVkSurface);
  if (aRes != VK_SUCCESS)
  {
    logFailureAndRelease ("unable to create surface", aRes);
    return false;
  }
#else
  int NOT_IMPLEMENTED = 0;
#endif
  if (myVkSurface == NULL)
  {
    return false;
  }

  VkBool32 isSupported = VK_FALSE;
  aRes = vkGetPhysicalDeviceSurfaceSupportKHR (theDevice->PhysicalDevice(), 0, myVkSurface, &isSupported);
  if (aRes != VK_SUCCESS || isSupported == VK_FALSE)
  {
    logFailureAndRelease ("surface not supported", aRes);
    return false;
  }
  
  mySize = CurrentSize();
  if (mySize.x() == 0
   || mySize.y() == 0)
  {
    return false;
  }

  if (!findFormat (theDevice, *myVkFormat))
  {
    Release();
    return false;
  }

  {
    VkSwapchainCreateInfoKHR aSwapChainInfo;
    aSwapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    aSwapChainInfo.pNext = NULL;
    aSwapChainInfo.flags = 0;
    aSwapChainInfo.surface = myVkSurface;
    aSwapChainInfo.minImageCount = 2;
    aSwapChainInfo.imageFormat = myVkFormat->format;
    aSwapChainInfo.imageColorSpace = myVkFormat->colorSpace;
    aSwapChainInfo.imageExtent.width  = mySize.x();
    aSwapChainInfo.imageExtent.height = mySize.y();
    aSwapChainInfo.imageArrayLayers = 1;
    aSwapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    aSwapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    aSwapChainInfo.queueFamilyIndexCount = 0;
    aSwapChainInfo.pQueueFamilyIndices = NULL;
    aSwapChainInfo.preTransform   = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    aSwapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    aSwapChainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;///theIsVSync ? VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
    aSwapChainInfo.clipped = VK_TRUE;
    aSwapChainInfo.oldSwapchain = VK_NULL_HANDLE;

    aRes = vkCreateSwapchainKHR (theDevice->Device(), &aSwapChainInfo, theDevice->HostAllocator(), &myVkSwapChain);
    if (aRes != VK_SUCCESS)
    {
      logFailureAndRelease ("failed to create swapchain", aRes);
      return false;
    }
  }

  {
    uint32_t aNbSwapChainImages = 0;
    aRes = vkGetSwapchainImagesKHR (theDevice->Device(), myVkSwapChain, &aNbSwapChainImages, NULL);
    if (aRes != VK_SUCCESS)
    {
      logFailureAndRelease ("failed to get swapchain images count", aRes);
      return false;
    }

    myVkImages.resize (aNbSwapChainImages, NULL);
    aRes = vkGetSwapchainImagesKHR (theDevice->Device(), myVkSwapChain, &aNbSwapChainImages, myVkImages.data());
    if (aRes != VK_SUCCESS)
    {
      logFailureAndRelease ("failed to get swapchain images", aRes);
      return false;
    }
  }

  myVkImageViews.resize (myVkImages.size(), NULL);
  for (uint32_t anImgIter = 0; anImgIter < myVkImages.size(); ++anImgIter)
  {
    VkImageViewCreateInfo aImgViewInfo = {};
    aImgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    aImgViewInfo.pNext = NULL;
    aImgViewInfo.flags = 0;
    aImgViewInfo.image = myVkImages[anImgIter];
    aImgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    aImgViewInfo.format = myVkFormat->format;
    aImgViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    aImgViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    aImgViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    aImgViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    aImgViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    aImgViewInfo.subresourceRange.baseMipLevel   = 0;
    aImgViewInfo.subresourceRange.levelCount     = 1;
    aImgViewInfo.subresourceRange.baseArrayLayer = 0;
    aImgViewInfo.subresourceRange.layerCount     = 1;

    aRes = vkCreateImageView (theDevice->Device(), &aImgViewInfo, theDevice->HostAllocator(), &myVkImageViews[anImgIter]);
    if (aRes != VK_SUCCESS)
    {
      logFailureAndRelease ("failed to create image view", aRes);
      return false;
    }
  }

  if (!myDepthImage->CreateDepthStencil (myDevice, mySize))
  {
    Release();
    return false;
  }

  return true;
}

// =======================================================================
// function : findFormat
// purpose  :
// =======================================================================
bool Vulkan_Surface::findFormat (const Handle(Vulkan_Device)& theDevice,
                                 VkSurfaceFormatKHR& theFormat)
{
  if (myVkSurface == NULL)
  {
    return false;
  }

  uint32_t aNbSurfFormats = 0;
  VkResult aRes = vkGetPhysicalDeviceSurfaceFormatsKHR (theDevice->PhysicalDevice(), myVkSurface, &aNbSurfFormats, NULL);
  if (aRes != VK_SUCCESS
   || aNbSurfFormats == 0)
  {
    logFailure ("failed to get surface formats count", aRes);
    return false;
  }

  std::vector<VkSurfaceFormatKHR> aVkSurfFormats (aNbSurfFormats);
  aRes = vkGetPhysicalDeviceSurfaceFormatsKHR (theDevice->PhysicalDevice(), myVkSurface, &aNbSurfFormats, aVkSurfFormats.data());
  if (aRes != VK_SUCCESS)
  {
    logFailure ("failed to get surface formats", aRes);
    return false;
  }

  theFormat = aVkSurfFormats[0];
  return true;
}

// =======================================================================
// function : CurrentSize
// purpose  :
// =======================================================================
Graphic3d_Vec2u Vulkan_Surface::CurrentSize()
{
  if (myVkSurface == NULL)
  {
    return Graphic3d_Vec2u (0, 0);
  }

  VkSurfaceCapabilitiesKHR aVkSurfCaps;
  VkResult aRes = vkGetPhysicalDeviceSurfaceCapabilitiesKHR (myDevice->PhysicalDevice(), myVkSurface, &aVkSurfCaps);
  if (aRes != VK_SUCCESS)
  {
    logFailure ("failed to get surface capabilities", aRes);
    return Graphic3d_Vec2u (0, 0);
  }

  return Graphic3d_Vec2u (aVkSurfCaps.currentExtent.width, aVkSurfCaps.currentExtent.height);
}

// =======================================================================
// function : AcquireNextImage
// purpose  :
// =======================================================================
bool Vulkan_Surface::AcquireNextImage (uint32_t& theSwapChainIndex)
{
  theSwapChainIndex = 0;
  if (myDevice.IsNull())
  {
    return false;
  }

  VkResult aRes = vkAcquireNextImageKHR (myDevice->Device(), myVkSwapChain, UINT64_MAX, VK_NULL_HANDLE, mySwapFence->Fence(), &theSwapChainIndex);
  if (aRes != VK_SUCCESS)
  {
    logFailure ("failed to get next swapchain image", aRes);
    return false;
  }

  return mySwapFence->Wait()
      && mySwapFence->Reset();
}
