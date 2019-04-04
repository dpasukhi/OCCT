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

#ifndef _Vulkan_Surface_HeaderFile
#define _Vulkan_Surface_HeaderFile

#include <Graphic3d_Vec2.hxx>
#include <Vulkan_Object.hxx>
#include <TCollection_AsciiString.hxx>

#include <memory>
#include <vector>

class Aspect_Window;
class Vulkan_Device;
class Vulkan_Fence;
class Vulkan_Image;

//! This class defines an Vulkan surface.
class Vulkan_Surface : public Vulkan_Object
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_Surface, Vulkan_Object)
public:

  //! Constructor.
  Standard_EXPORT Vulkan_Surface();

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_Surface();

  //! Perform initialization.
  Standard_EXPORT bool Init (const Handle(Vulkan_Device)& theDevice,
                             const Handle(Aspect_Window)& theWindow);

  //! Return surface.
  VkSurfaceKHR Surface() const { return myVkSurface; }

  //! Return swap chain.
  VkSwapchainKHR SwapChain() const { return myVkSwapChain; }

  //! Return the swap chain length.
  uint32_t SwapChainSize() const { return (uint32_t )myVkImageViews.size(); }

  //! Return images within swap chain.
  const std::vector<VkImage>& Images() const { return myVkImages; }

  //! Return image views within swap chain.
  const std::vector<VkImageView>& ImageViews() const { return myVkImageViews; }

  //! Return color surface format.
  const VkSurfaceFormatKHR& ColorFormat() const { return *myVkFormat; }

  //! Return depth image.
  const Handle(Vulkan_Image)& DepthImage() const { return myDepthImage; }

  //! Return surface size.
  const Graphic3d_Vec2u& Size() const { return mySize; }

  //! Fetch actual surface size.
  Graphic3d_Vec2u CurrentSize();

  //! Acquire next image from swap chain, @sa vkAcquireNextImageKHR().
  Standard_EXPORT bool AcquireNextImage (uint32_t& theSwapChainIndex);

protected:

  //! Find supported format.
  Standard_EXPORT bool findFormat (const Handle(Vulkan_Device)& theDevice,
                                   VkSurfaceFormatKHR& theFormat);

  //! Release the object.
  virtual void release() Standard_OVERRIDE { releaseSurface(); }

  //! Release the object, @sa vkDestroySurfaceKHR().
  Standard_EXPORT void releaseSurface();

protected:

  Handle(Vulkan_Fence) mySwapFence;
  Handle(Vulkan_Image) myDepthImage;
  VkSurfaceKHR         myVkSurface;
  VkSwapchainKHR       myVkSwapChain;
  Graphic3d_Vec2u      mySize;
  std::vector<VkImage>     myVkImages;
  std::vector<VkImageView> myVkImageViews;
  std::shared_ptr<VkSurfaceFormatKHR> myVkFormat;

};

#endif // _Vulkan_Surface_HeaderFile
