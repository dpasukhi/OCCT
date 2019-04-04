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

#ifndef _Vulkan_DeviceMemory_HeaderFile
#define _Vulkan_DeviceMemory_HeaderFile

#include <Vulkan_Object.hxx>

class Vulkan_DeviceMemoryAllocator;

//! Device memory region information.
struct Vulkan_DeviceMemoryInfo
{
  VkDeviceMemory DeviceMemory;
  VkDeviceSize   Offset;
  VkDeviceSize   Size;

  Vulkan_DeviceMemoryInfo() : DeviceMemory (NULL), Offset (0), Size (0) {}
};

//! This class defines an device memory block.
class Vulkan_DeviceMemory : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_DeviceMemory, Standard_Transient)
  friend class Vulkan_DeviceMemoryAllocator;
public:

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_DeviceMemory();

  //! Return memory region.
  Standard_EXPORT Vulkan_DeviceMemoryInfo DeviceMemoryInfo() const;

protected:

  //! Constructor.
  Standard_EXPORT Vulkan_DeviceMemory (const Handle(Vulkan_DeviceMemoryAllocator)& theAllocator,
                                       void* theOpaque);

  //! Release the object.
  void release() { releaseRegion(); }

  //! Release the object.
  Standard_EXPORT void releaseRegion();

protected:

  Handle(Vulkan_DeviceMemoryAllocator) myAllocator;
  void* myOpaque;

};

#endif // _Vulkan_DeviceMemory_HeaderFile
