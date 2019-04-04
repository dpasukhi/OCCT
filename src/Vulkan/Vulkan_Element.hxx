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

#ifndef _Vulkan_Element_HeaderFile
#define _Vulkan_Element_HeaderFile

#include <Standard_Type.hxx>

class Vulkan_Context;

//! Base interface for drawable elements.
class Vulkan_Element : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_Element, Standard_Transient)
public:

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_Element();

  //! Release GPU resources.
  virtual void Release() = 0;

  //! Perform rendering.
  virtual void Render (const Handle(Vulkan_Context)& theCtx) = 0;

public:

  //! Return TRUE if primitive type generates shaded triangulation (to be used in filters).
  virtual Standard_Boolean IsFillDrawMode() const { return false; }

  //! Update parameters of the drawable elements.
  virtual void SynchronizeAspects (const Handle(Vulkan_Context)& ) {}

protected:

  //! Empty constructor.
  Standard_EXPORT Vulkan_Element();

};

#endif // _Vulkan_Element_HeaderFile
