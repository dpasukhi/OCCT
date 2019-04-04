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

#ifndef _Vulkan_Fence_HeaderFile
#define _Vulkan_Fence_HeaderFile

#include <Vulkan_Object.hxx>

//! This class defines an Vulkan fence.
class Vulkan_Fence : public Vulkan_Object
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_Fence, Vulkan_Object)
public:

  //! Constructor.
  Standard_EXPORT Vulkan_Fence();

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_Fence();

  //! Return object.
  VkFence Fence() const { return myVkFence; }

  //! Create the object, @sa vkCreateFence().
  Standard_EXPORT bool Create (const Handle(Vulkan_Device)& theDevice);

  //! Wait for the fence, @sa vkWaitForFences().
  Standard_EXPORT bool Wait();

  //! Reset the fence, @sa vkResetFences().
  Standard_EXPORT bool Reset();

protected:

  //! Release the object.
  virtual void release() Standard_OVERRIDE { releaseFence(); }

  //! Release the object, @sa vkDestroyFence().
  Standard_EXPORT void releaseFence();

protected:

  VkFence myVkFence;

};

#endif // _Vulkan_Fence_HeaderFile
