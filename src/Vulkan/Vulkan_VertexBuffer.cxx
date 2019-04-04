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

#include <Vulkan_VertexBuffer.hxx>

#include <Graphic3d_Buffer.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_VertexBuffer, Vulkan_Buffer)

// =======================================================================
// function : Vulkan_VertexBuffer
// purpose  :
// =======================================================================
Vulkan_VertexBuffer::Vulkan_VertexBuffer()
{
  //
}

// =======================================================================
// function : Init
// purpose  :
// =======================================================================
bool Vulkan_VertexBuffer::Init (const Handle(Vulkan_Device)& theDevice,
                                const Handle(Graphic3d_Buffer)& theAttribs)
{
  if (theAttribs.IsNull())
  {
    Release();
    return false;
  }
  return Init (theDevice, theAttribs->Data(), theAttribs->Size());
}
