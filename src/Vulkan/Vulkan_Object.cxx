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

#include <Vulkan_Object.hxx>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_Device.hxx>

#include <vulkan/vulkan.h>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_Object, Standard_Transient)

// =======================================================================
// function : Vulkan_Object
// purpose  :
// =======================================================================
Vulkan_Object::Vulkan_Object()
{
  //
}

// =======================================================================
// function : ~Vulkan_Object
// purpose  :
// =======================================================================
Vulkan_Object::~Vulkan_Object()
{
  //
}

// =======================================================================
// function : logFailure
// purpose  :
// =======================================================================
void Vulkan_Object::logFailure (const TCollection_AsciiString& theMsg,
                                int theVkErr,
                                bool theToRelease)
{
  if (theVkErr != VK_SUCCESS)
  {
    Message::DefaultMessenger()->Send (TCollection_AsciiString (DynamicType()->Name()) + ", " + theMsg + ": " + Vulkan_Device::FormatVkError (theVkErr));
  }
  else
  {
    Message::DefaultMessenger()->Send (TCollection_AsciiString (DynamicType()->Name()) + ", " + theMsg);
  }
  if (theToRelease)
  {
    Release();
  }
}
