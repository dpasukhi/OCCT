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

#include <Vulkan_Caps.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_Caps, Standard_Transient)

// =======================================================================
// function : Vulkan_Caps
// purpose  :
// =======================================================================
Vulkan_Caps::Vulkan_Caps()
: swapInterval      (1),
  buffersNoSwap     (Standard_False),
#ifdef OCCT_DEBUG
  contextDebug      (Standard_True),
#else
  contextDebug      (Standard_False),
#endif
  contextNoAccel    (Standard_False),
  glslWarnings      (Standard_False),
  suppressExtraMsg  (Standard_True)
{
  //
}

// =======================================================================
// function : operator=
// purpose  :
// =======================================================================
Vulkan_Caps& Vulkan_Caps::operator= (const Vulkan_Caps& theCopy)
{
  swapInterval      = theCopy.swapInterval;
  buffersNoSwap     = theCopy.buffersNoSwap;
  contextDebug      = theCopy.contextDebug;
  contextNoAccel    = theCopy.contextNoAccel;
  contextDevice     = theCopy.contextDevice;
  glslWarnings      = theCopy.glslWarnings;
  suppressExtraMsg  = theCopy.suppressExtraMsg;
  return *this;
}

// =======================================================================
// function : ~Vulkan_Caps
// purpose  :
// =======================================================================
Vulkan_Caps::~Vulkan_Caps()
{
  //
}
