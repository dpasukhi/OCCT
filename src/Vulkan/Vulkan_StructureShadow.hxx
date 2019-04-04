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

#ifndef _Vulkan_StructureShadow_HeaderFile
#define _Vulkan_StructureShadow_HeaderFile

#include <Vulkan_Structure.hxx>

//! Dummy structure which just redirects to groups of another structure.
class Vulkan_StructureShadow : public Vulkan_Structure
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_StructureShadow, Vulkan_Structure)
public:

  //! Create empty structure
  Standard_EXPORT Vulkan_StructureShadow (const Handle(Graphic3d_StructureManager)& theManager,
                                          const Handle(Vulkan_Structure)& theStructure);

public:

  //! Raise exception on API misuse.
  virtual void Connect (Graphic3d_CStructure& ) Standard_OVERRIDE;

  //! Raise exception on API misuse.
  virtual void Disconnect (Graphic3d_CStructure& ) Standard_OVERRIDE;

private:

  Handle(Vulkan_Structure) myParent;

};

#endif // _Vulkan_StructureShadow_HeaderFile
