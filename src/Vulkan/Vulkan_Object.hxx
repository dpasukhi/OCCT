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

#ifndef _Vulkan_Object_HeaderFile
#define _Vulkan_Object_HeaderFile

#include <Vulkan_ForwardDecl.hxx>
#include <Standard_Type.hxx>

class Vulkan_Device;
class TCollection_AsciiString;

//! Interface defining a Vulkan object.
//! After initialization, object holds Vulkan_Device instance, so that it can be automatically released on destruction.
//! Beware that Vulkan object might hold graphics memory, so that not releasing it at proper time could lead to memory issues.
class Vulkan_Object : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_Object, Standard_Transient)
public:

  //! Constructor.
  Standard_EXPORT Vulkan_Object();

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_Object();

  //! Release the object.
  void Release() { release(); }

  //! Return the Vulkan device.
  const Handle(Vulkan_Device)& Device() const { return myDevice; }

protected:

  //! Release the object.
  virtual void release() = 0;

  //! Log failure into messenger and release the object.
  void logFailureAndRelease (const TCollection_AsciiString& theMsg,
                             int theVkErr)
  {
    logFailure (theMsg, theVkErr, true);
  }

  //! Log failure into messenger.
  void logFailure (const TCollection_AsciiString& theMsg,
                   int theVkErr,
                   bool theToRelease = false);

  //! @def Vulkan_AssertOnRelease
  //! Auxiliary macros to check that Vulkan object has been released in proper order.
#ifdef _DEBUG
  #define Vulkan_AssertOnRelease(theName) \
    Standard_ASSERT_RETURN(!myDevice.IsNull(),theName##" destroyed without Vulkan device",); \
    Standard_ASSERT_RETURN(myDevice->Device()!=NULL,theName##"Vulkan_Buffer destroyed after Vulkan device destruction",);
#else
  #define Vulkan_AssertOnRelease
#endif

protected:

  Handle(Vulkan_Device) myDevice;

};

#endif // _Vulkan_Object_HeaderFile
