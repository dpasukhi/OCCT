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

#ifndef _Vulkan_Image_HeaderFile
#define _Vulkan_Image_HeaderFile

#include <Graphic3d_Vec2.hxx>
#include <Vulkan_Object.hxx>

#include <memory>

class Vulkan_DeviceMemory;

//! This class defines an Vulkan image.
class Vulkan_Image : public Vulkan_Object
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_Image, Vulkan_Object)
public:

  //! Constructor.
  Standard_EXPORT Vulkan_Image();

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_Image();

  //! Return image.
  VkImage Image() const { return myVkImage; }

  //! Return image view.
  VkImageView ImageView() const { return myVkImageView; }

  //! Return image size.
  const Graphic3d_Vec2u& Size() const { return mySize; }

  //! Return color surface format.
  const VkSurfaceFormatKHR& SurfaceFormat() const { return *myVkFormat; }

  //! Create the object, @sa vkCreateImage(), vkCreateImageView().
  Standard_EXPORT bool CreateDepthStencil (const Handle(Vulkan_Device)& theDevice,
                                           const Graphic3d_Vec2u& theSize);

protected:

  //! Release the object.
  virtual void release() Standard_OVERRIDE { releaseImage(); }

  //! Release the object, @sa vkDestroyImage().
  Standard_EXPORT void releaseImage();

protected:

  Handle(Vulkan_DeviceMemory) myDepthMemory;
  std::shared_ptr<VkSurfaceFormatKHR> myVkFormat;
  VkImage         myVkImage;
  VkImageView     myVkImageView;
  Graphic3d_Vec2u mySize;

};

#endif // _Vulkan_Image_HeaderFile
