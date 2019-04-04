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

#include <Vulkan_Buffer.hxx>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_Device.hxx>
#include <Vulkan_DeviceMemory.hxx>
#include <Vulkan_DeviceMemoryAllocator.hxx>

#include <vulkan/vulkan.h>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_Buffer, Vulkan_Object)

// =======================================================================
// function : Vulkan_Buffer
// purpose  :
// =======================================================================
Vulkan_Buffer::Vulkan_Buffer()
: myVkBuffer (NULL),
  mySize (0)
{
  //
}

// =======================================================================
// function : ~Vulkan_Buffer
// purpose  :
// =======================================================================
Vulkan_Buffer::~Vulkan_Buffer()
{
  releaseBuffer();
}

// =======================================================================
// function : releaseBuffer
// purpose  :
// =======================================================================
void Vulkan_Buffer::releaseBuffer()
{
  mySize = 0;
  if (myVkBuffer != NULL)
  {
    Vulkan_AssertOnRelease("Vulkan_Buffer");
    vkDestroyBuffer (myDevice->Device(), myVkBuffer, myDevice->HostAllocator());
    myVkBuffer = NULL;
  }
  if (!myDevMemory.IsNull())
  {
    myDevMemory.Nullify();
  }
  myDevice.Nullify();
}

// =======================================================================
// function : create
// purpose  :
// =======================================================================
bool Vulkan_Buffer::create (const Handle(Vulkan_Device)& theDevice,
                            Standard_Size theSize,
                            Vulkan_BufferType theType)
{
  if (myVkBuffer != NULL
   && mySize == theSize
   && myDevice == theDevice)
  {
    return true;
  }

  Release();
  if (theDevice.IsNull()
   || theDevice->Device() == NULL)
  {
    return false;
  }

  myDevice = theDevice;

  VkBufferCreateInfo aVkBuffInfo;
  aVkBuffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  aVkBuffInfo.pNext = NULL;
  aVkBuffInfo.flags = 0;
  aVkBuffInfo.size = theSize;
  aVkBuffInfo.usage = 0;
  aVkBuffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  aVkBuffInfo.queueFamilyIndexCount = 0;
  aVkBuffInfo.pQueueFamilyIndices = NULL;
  switch (theType)
  {
    case Vulkan_BufferType_Uniform: aVkBuffInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; break;
    case Vulkan_BufferType_Vertex:  aVkBuffInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;  break;
    case Vulkan_BufferType_Index:   aVkBuffInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;   break;
  }

  VkResult aRes = vkCreateBuffer (theDevice->Device(), &aVkBuffInfo, theDevice->HostAllocator(), &myVkBuffer);
  if (aRes != VK_SUCCESS)
  {
    logFailureAndRelease ("failed to create buffer", aRes);
    return false;
  }

  mySize = theSize;
  return true;
}

// =======================================================================
// function : init
// purpose  :
// =======================================================================
bool Vulkan_Buffer::init (const Handle(Vulkan_Device)& theDevice,
                          const void* theData,
                          Standard_Size theNbBytes,
                          Standard_Size theNbBytesFull,
                          Vulkan_BufferType theType)
{
  if (!create (theDevice, theNbBytesFull != 0 ? theNbBytesFull : theNbBytes, theType))
  {
    return false;
  }

  const bool toAllocMemory = myDevMemory.IsNull();
  if (toAllocMemory)
  {
    VkMemoryRequirements aVkMemReqs = {};
    vkGetBufferMemoryRequirements (theDevice->Device(), myVkBuffer, &aVkMemReqs);
    myDevMemory = theDevice->DeviceMemoryAllocator()->Allocate (aVkMemReqs, Vulkan_DeviceMemoryUsage_CpuToGpu);
    if (myDevMemory.IsNull())
    {
      Release();
      return false;
    }
  }

  if (theData != NULL)
  {
    void* aDataPtr = NULL;
    const Vulkan_DeviceMemoryInfo aMemInfo = myDevMemory->DeviceMemoryInfo();
    VkResult aRes = vkMapMemory (theDevice->Device(), aMemInfo.DeviceMemory, aMemInfo.Offset, theNbBytes, 0, &aDataPtr);
    if (aRes != VK_SUCCESS)
    {
      logFailureAndRelease ("failed to map device memory", aRes);
      return false;
    }
    memcpy (aDataPtr, theData, theNbBytes);
    vkUnmapMemory (theDevice->Device(), aMemInfo.DeviceMemory);
  }

  if (toAllocMemory)
  {
    const Vulkan_DeviceMemoryInfo aMemInfo = myDevMemory->DeviceMemoryInfo();
    VkResult aRes = vkBindBufferMemory (theDevice->Device(), myVkBuffer, aMemInfo.DeviceMemory, aMemInfo.Offset);
    if (aRes != VK_SUCCESS)
    {
      logFailureAndRelease ("failed to bind buffer memory", aRes);
      return false;
    }
  }

  return true;
}
