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

#ifndef _Vulkan_DeviceMemoryAllocator_HeaderFile
#define _Vulkan_DeviceMemoryAllocator_HeaderFile

#include <Vulkan_Object.hxx>

VK_DEFINE_HANDLE(VmaAllocator)
class Vulkan_DeviceMemory;
struct Vulkan_DeviceMemoryInfo;

enum Vulkan_DeviceMemoryUsage
{
  Vulkan_DeviceMemoryUsage_UNKNOWN = -1, //!< unknown memory usage
  Vulkan_DeviceMemoryUsage_GpuOnly = 0,  //!< memory to be used on device only
  Vulkan_DeviceMemoryUsage_CpuOnly,      //!< memory to be mappable on host
  Vulkan_DeviceMemoryUsage_CpuToGpu,     //!< memory that is both mappable on host and preferably fast to access by GPU
  Vulkan_DeviceMemoryUsage_GpuToCpu,     //!< memory mappable on host and cached
};

//! This class defines a memory allocator for device memory.
//! It is expected to be created by Vulkan_Device itself.
class Vulkan_DeviceMemoryAllocator : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_DeviceMemoryAllocator, Standard_Transient)
  friend class Vulkan_DeviceMemory;
  friend class Vulkan_Device;
public:

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_DeviceMemoryAllocator();

  //! Allocate new memory region.
  Handle(Vulkan_DeviceMemory) Allocate (const VkMemoryRequirements& theReqs,
                                        Vulkan_DeviceMemoryUsage theUsage);

protected:

  //! Constructor.
  Standard_EXPORT Vulkan_DeviceMemoryAllocator();

  //! Create the object.
  Standard_EXPORT bool Create (const Handle(Vulkan_Device)& theDevice,
                               bool toUseDedicatedAllocs);

  //! Release the object.
  void release() { releaseAllocator(); }

  //! Release the object.
  Standard_EXPORT void releaseAllocator();

  //! Return memory region information.
  Standard_EXPORT Vulkan_DeviceMemoryInfo memoryRegionInfo (const Vulkan_DeviceMemory& theMem) const;

  //! Release memory region.
  Standard_EXPORT void memoryRegionFree (Vulkan_DeviceMemory& theMem);

protected:

  VmaAllocator   myVmaAllocator;
  Vulkan_Device* myDevice;

};

#endif // _Vulkan_DeviceMemoryAllocator_HeaderFile
