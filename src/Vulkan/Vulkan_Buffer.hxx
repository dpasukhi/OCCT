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

#ifndef _Vulkan_Buffer_HeaderFile
#define _Vulkan_Buffer_HeaderFile

#include <Graphic3d_Vec.hxx>
#include <Vulkan_Object.hxx>

class Vulkan_DeviceMemory;

enum Vulkan_BufferType
{
  Vulkan_BufferType_Uniform, //!< VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
  Vulkan_BufferType_Vertex,  //!< VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
  Vulkan_BufferType_Index,   //!< VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
};

//! This class defines an Vulkan buffer.
class Vulkan_Buffer : public Vulkan_Object
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_Buffer, Vulkan_Object)
public:

  //! Constructor.
  Standard_EXPORT Vulkan_Buffer();

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_Buffer();

  //! Return object.
  VkBuffer Buffer() const { return myVkBuffer; }

  //! Return device memory object.
  const Handle(Vulkan_DeviceMemory)& DeviceMemory() const { return myDevMemory; }

protected:

  //! Create the object, @sa vkCreateBuffer().
  Standard_EXPORT bool create (const Handle(Vulkan_Device)& theDevice,
                               Standard_Size theSize,
                               Vulkan_BufferType theType);

  //! Init the object.
  bool init (const Handle(Vulkan_Device)& theDevice,
             const void* theData,
             Standard_Size theNbBytes,
             Vulkan_BufferType theType)
  {
    return init (theDevice, theData, theNbBytes, 0, theType);
  }

  //! Init the object.
  Standard_EXPORT bool init (const Handle(Vulkan_Device)& theDevice,
                             const void* theData,
                             Standard_Size theNbBytes,
                             Standard_Size theNbBytesFull,
                             Vulkan_BufferType theType);

  //! Release the object.
  virtual void release() Standard_OVERRIDE { releaseBuffer(); }

  //! Release the object, @sa vkDestroyBuffer().
  Standard_EXPORT void releaseBuffer();

protected:

  Handle(Vulkan_DeviceMemory) myDevMemory;
  VkBuffer       myVkBuffer;
  Standard_Size  mySize;

};

#endif // _Vulkan_Buffer_HeaderFile
