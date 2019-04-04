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

#ifndef _Vulkan_FrameBuffer_HeaderFile
#define _Vulkan_FrameBuffer_HeaderFile

#include <Graphic3d_Vec2.hxx>
#include <Vulkan_Object.hxx>

class Vulkan_Image;
class Vulkan_RenderPass;
class Vulkan_Surface;

//! This class defines an Vulkan frame buffer.
class Vulkan_FrameBuffer : public Vulkan_Object
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_FrameBuffer, Vulkan_Object)
public:

  //! Constructor.
  Standard_EXPORT Vulkan_FrameBuffer();

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_FrameBuffer();

  //! Return object.
  VkFramebuffer FrameBuffer() const { return myVkFramebuffer; }

  //! Create the object, @sa vkCreateFramebuffer().
  Standard_EXPORT bool Create (const Handle(Vulkan_RenderPass)& theRenderPass,
                               const Handle(Vulkan_Surface)& theSurface,
                               const uint32_t theChainIndex);

protected:

  //! Release the object.
  virtual void release() Standard_OVERRIDE { releaseBuffer(); }

  //! Release the object, @sa vkDestroyFramebuffer().
  Standard_EXPORT void releaseBuffer();

protected:

  Handle(Vulkan_RenderPass) myRenderPass;
  Handle(Vulkan_Surface)    mySurface;
  Handle(Vulkan_Image)      myDepth;
  VkFramebuffer   myVkFramebuffer;
  VkImageView     myVkImageView;
  Graphic3d_Vec2u mySize;

};

#endif // _Vulkan_FrameBuffer_HeaderFile
