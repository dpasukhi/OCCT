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

#ifndef _Vulkan_CommandPool_HeaderFile
#define _Vulkan_CommandPool_HeaderFile

#include <Vulkan_Object.hxx>

class Vulkan_CommandBuffer;

//! This class defines an Vulkan command pool.
class Vulkan_CommandPool : public Vulkan_Object
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_CommandPool, Vulkan_Object)
public:

  //! Constructor.
  Standard_EXPORT Vulkan_CommandPool();

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_CommandPool();

  //! Return object.
  VkCommandPool CommandPool() const { return myVkCmdPool; }

  //! Create the object, @sa vkCreateCommandPool().
  Standard_EXPORT bool Create (const Handle(Vulkan_Device)& theDevice);

  //! Reset command pool, @sa vkResetCommandPool().
  Standard_EXPORT bool ResetPool();

  //! Allocate single command buffer from this pool, @sa vkAllocateCommandBuffers().
  Handle(Vulkan_CommandBuffer) AllocateBuffer();

protected:

  //! Release the object.
  virtual void release() Standard_OVERRIDE { releasePool(); }

  //! Release the object, @sa vkDestroyCommandPool().
  Standard_EXPORT void releasePool();

protected:

  VkCommandPool myVkCmdPool;

};

#endif // _Vulkan_CommandPool_HeaderFile
