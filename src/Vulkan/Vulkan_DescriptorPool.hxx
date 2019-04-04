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

#ifndef _Vulkan_DescriptorPool_HeaderFile
#define _Vulkan_DescriptorPool_HeaderFile

#include <Vulkan_Object.hxx>

//! This class defines an Vulkan descriptor pool.
class Vulkan_DescriptorPool : public Vulkan_Object
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_DescriptorPool, Vulkan_Object)
public:

  //! Constructor.
  Standard_EXPORT Vulkan_DescriptorPool();

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_DescriptorPool();

  //! Return object.
  VkDescriptorPool DescriptorPool() const { return myVkDescPool; }

  //! Create the object, @sa vkCreateDescriptorPool().
  Standard_EXPORT bool Create (const Handle(Vulkan_Device)& theDevice);

  //! Reset the fence, @sa vkResetDescriptorPool().
  Standard_EXPORT bool ResetPool();

protected:

  //! Release the object.
  virtual void release() Standard_OVERRIDE { releasePool(); }

  //! Release the object, @sa vkDestroyDescriptorPool().
  Standard_EXPORT void releasePool();

protected:

  VkDescriptorPool myVkDescPool;

};

#endif // _Vulkan_DescriptorPool_HeaderFile
