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

#include <Vulkan_DeviceMemory.hxx>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_Device.hxx>
#include <Vulkan_DeviceMemoryAllocator.hxx>

#include <vulkan/vulkan.h>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_DeviceMemory, Vulkan_Object)

// =======================================================================
// function : Vulkan_DeviceMemory
// purpose  :
// =======================================================================
Vulkan_DeviceMemory::Vulkan_DeviceMemory (const Handle(Vulkan_DeviceMemoryAllocator)& theAllocator,
                                          void* theOpaque)
: myAllocator (theAllocator),
  myOpaque (theOpaque)
{
  //
}

// =======================================================================
// function : ~Vulkan_DeviceMemory
// purpose  :
// =======================================================================
Vulkan_DeviceMemory::~Vulkan_DeviceMemory()
{
  releaseRegion();
}

// =======================================================================
// function : DeviceMemoryInfo
// purpose  :
// =======================================================================
Vulkan_DeviceMemoryInfo Vulkan_DeviceMemory::DeviceMemoryInfo() const
{
  return myOpaque != NULL ? myAllocator->memoryRegionInfo (*this) : Vulkan_DeviceMemoryInfo();
}

// =======================================================================
// function : releaseRegion
// purpose  :
// =======================================================================
void Vulkan_DeviceMemory::releaseRegion()
{
  if (myOpaque != NULL)
  {
    myAllocator->memoryRegionFree (*this);
  }
  myAllocator.Nullify();
}
