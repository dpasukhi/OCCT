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

#ifndef _Vulkan_UniformBuffer_HeaderFile
#define _Vulkan_UniformBuffer_HeaderFile

#include <Vulkan_Buffer.hxx>

class Graphic3d_Buffer;

//! This class defines an Vulkan buffer.
class Vulkan_UniformBuffer : public Vulkan_Buffer
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_UniformBuffer, Vulkan_Buffer)
public:

  //! Constructor.
  Standard_EXPORT Vulkan_UniformBuffer();

public:

  //! Create the object, @sa vkCreateBuffer().
  bool Create (const Handle(Vulkan_Device)& theDevice,
               Standard_Size theSize)
  {
    return create (theDevice, theSize, Vulkan_BufferType_Uniform);
  }

  //! Init the object.
  bool Init (const Handle(Vulkan_Device)& theDevice,
             const void* theData,
             Standard_Size theNbBytes,
             Standard_Size theNbBytesFull = 0)
  {
    return init (theDevice, theData, theNbBytes, theNbBytesFull, Vulkan_BufferType_Uniform);
  }

};

#endif // _Vulkan_UniformBuffer_HeaderFile
