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

#ifndef _Vulkan_Device_HeaderFile
#define _Vulkan_Device_HeaderFile

#include <Graphic3d_DiagnosticInfo.hxx>
#include <Vulkan_ForwardDecl.hxx>
#include <Standard_Type.hxx>
#include <TCollection_AsciiString.hxx>
#include <TColStd_IndexedDataMapOfStringString.hxx>

#include <memory>

class Vulkan_Caps;
class Vulkan_FrameStats;
class Vulkan_DeviceMemoryAllocator;

//! This class defines an Vulkan graphic driver
class Vulkan_Device : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_Device, Standard_Transient)
public:

  //! Format VkResult enumeration.
  Standard_EXPORT static TCollection_AsciiString FormatVkError (int theErr);

  //! Enumeration of known vendor ids.
  enum VendorId
  {
    VendorId_AMD    = 0x1002,
    VendorId_NVIDIA = 0x10DE,
    VendorId_INTEL  = 0x8086,
  };

public:

  //! Constructor.
  //! @param theAppName    application    name to be passed to driver
  //! @param theAppVersion application version to be passed to driver, see VK_MAKE_VERSION() macros
  //! @param theEngineName      engine    name to be passed to driver
  //! @param theEngineVersion   engine version to be passed to driver, see VK_MAKE_VERSION() macros
  Standard_EXPORT Vulkan_Device (const TCollection_AsciiString& theAppName,
                                 const uint32_t theAppVersion,
                                 const TCollection_AsciiString& theEngineName,
                                 const uint32_t theEngineVersion);

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_Device();

  //! Release object.
  Standard_EXPORT void Release();

  //! Perform initialization.
  Standard_EXPORT bool Init (const Handle(Vulkan_Caps)& theCaps);

  //! Return vulkan instance.
  VkInstance Instance() const { return myVkInstance; }
  
  //! Return device.
  VkDevice Device() const { return myVkDevice; }

  //! Return physical device.
  VkPhysicalDevice PhysicalDevice() const { return myVkPhysDevice; }

  //! Return custom allocator.
  const VkAllocationCallbacks* HostAllocator() const { return myVkHostAllocator.get(); }

  //! Return application identifier specified at construction time.
  const TCollection_AsciiString& ApplicationName() const { return myAppName; }

  //! Return engine identifier specified at construction time.
  const TCollection_AsciiString& EngineName() const { return myEngineName; }
  
  //! Return application version specified at construction time.
  uint32_t ApplicationVersion() const { return myAppVersion; }

  //! Return engine version specified at construction time.
  uint32_t EngineVersion() const { return myEngineVersion; }

  //! Return frame stats.
  const Handle(Vulkan_FrameStats)& FrameStats() const { return myFrameStats; }

  //! Return device memory allocator.
  const Handle(Vulkan_DeviceMemoryAllocator)& DeviceMemoryAllocator() const { return myDevMemAllocator; }

  //! Allocate device memory.
  Standard_EXPORT VkDeviceMemory allocateDeviceMemory (const VkMemoryRequirements& theReqs);

  //! Return VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment property.
  VkDeviceSize MinUniformBufferOffsetAlignment() const { return myMinUniformBufferOffsetAlignment; }

  //! Fill map with diagnostics information.
  Standard_EXPORT void DiagnosticInformation (TColStd_IndexedDataMapOfStringString& theDict,
                                              Graphic3d_DiagnosticInfo theFlags) const;

protected:

  TCollection_AsciiString   myAppName;         //!< application identifier to be passed to driver
  TCollection_AsciiString   myEngineName;      //!<      engine identifier to be passed to driver
  uint32_t                  myAppVersion;      //!< application    version to be passed to driver
  uint32_t                  myEngineVersion;   //!<      engine    version to be passed to driver

  VkInstance                myVkInstance;      //!< vulkan instance
  VkPhysicalDevice          myVkPhysDevice;    //!< physical device
  VkDevice                  myVkDevice;        //!< device
  std::shared_ptr<VkAllocationCallbacks>
                            myVkHostAllocator; //!< optional host memory allocator
  std::shared_ptr<VkPhysicalDeviceMemoryProperties>
                            myVkDeviceMemory;
  VkDeviceSize              myMinUniformBufferOffsetAlignment;
  Handle(Vulkan_FrameStats) myFrameStats;
  Handle(Vulkan_DeviceMemoryAllocator) myDevMemAllocator;

};

#endif // _Vulkan_Device_HeaderFile
