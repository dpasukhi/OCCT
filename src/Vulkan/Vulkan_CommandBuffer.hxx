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

#ifndef _Vulkan_CommandBuffer_HeaderFile
#define _Vulkan_CommandBuffer_HeaderFile

#include <Graphic3d_Vec.hxx>
#include <Vulkan_Object.hxx>

class Vulkan_CommandPool;
class Vulkan_FrameBuffer;
class Vulkan_Pipeline;
class Vulkan_RenderPass;

//! This class defines an Vulkan command buffer.
class Vulkan_CommandBuffer : public Vulkan_Object
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_CommandBuffer, Vulkan_Object)
public:

  //! Constructor.
  Standard_EXPORT Vulkan_CommandBuffer();

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_CommandBuffer();

  //! Return object.
  const VkCommandBuffer& CommandBuffer() const { return myVkCmdBuffer; }

  //! Create the object, @sa vkAllocateCommandBuffers().
  Standard_EXPORT bool Create (const Handle(Vulkan_CommandPool)& thePool);

  //! Reset commands in this buffer, @sa vkResetCommandBuffer().
  Standard_EXPORT bool ResetCommandBuffer();

  //! Begin writing command buffer, @sa vkBeginCommandBuffer().
  Standard_EXPORT void BeginCommandBuffer (const Handle(Vulkan_RenderPass)& theRenderPass,
                                           const Handle(Vulkan_FrameBuffer)& theFrameBuffer);

  //! End writing command buffer, @sa vkEndCommandBuffer().
  Standard_EXPORT void EndCommandBuffer();

  //! Begin render pass, @sa vkCmdBeginRenderPass().
  Standard_EXPORT void BeginRenderPass (const Handle(Vulkan_RenderPass)& theRenderPass,
                                        const Handle(Vulkan_FrameBuffer)& theFrameBuffer,
                                        const Graphic3d_Vec2u& theSize,
                                        const Graphic3d_Vec4* theClearColor);

  //! End render pass, @sa vkCmdEndRenderPass().
  Standard_EXPORT void EndRenderPass();

  //! Bind the pipeline, @sa vkCmdBindPipeline().
  Standard_EXPORT void BindPipeline (const Handle(Vulkan_Pipeline)& thePipeline);

protected:

  //! Release the object.
  virtual void release() Standard_OVERRIDE { releaseBuffer(); }

  //! Release the object, @sa vkFreeCommandBuffers().
  Standard_EXPORT void releaseBuffer();

protected:

  Handle(Vulkan_CommandPool) myCmdPool;
  VkCommandBuffer myVkCmdBuffer;

};

#endif // _Vulkan_CommandBuffer_HeaderFile
