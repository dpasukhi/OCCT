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

#include <Vulkan_IndexBuffer.hxx>

#include <Graphic3d_IndexBuffer.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_IndexBuffer, Vulkan_Buffer)

// =======================================================================
// function : Vulkan_IndexBuffer
// purpose  :
// =======================================================================
Vulkan_IndexBuffer::Vulkan_IndexBuffer()
: myStride (0),
  myNbElements (0)
{
  //
}

// =======================================================================
// function : Init
// purpose  :
// =======================================================================
bool Vulkan_IndexBuffer::Init (const Handle(Vulkan_Device)& theDevice,
                               const Handle(Graphic3d_IndexBuffer)& theIndices)
{
  if (theIndices.IsNull())
  {
    Release();
    return false;
  }
  if (!init (theDevice, theIndices->Data(), theIndices->Size(), Vulkan_BufferType_Index))
  {
    return false;
  }

  myStride     = theIndices->Stride;
  myNbElements = theIndices->NbElements;
  return true;
}
