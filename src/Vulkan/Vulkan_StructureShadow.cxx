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

#include <Vulkan_StructureShadow.hxx>

#include <Graphic3d_GraphicDriver.hxx>
#include <Standard_ProgramError.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_StructureShadow, Vulkan_Structure)

//=======================================================================
//function : Vulkan_StructureShadow
//purpose  :
//=======================================================================
Vulkan_StructureShadow::Vulkan_StructureShadow (const Handle(Graphic3d_StructureManager)& theManager,
                                                const Handle(Vulkan_Structure)& theStructure)
: Vulkan_Structure (theManager)
{
  Handle(Vulkan_StructureShadow) aShadow = Handle(Vulkan_StructureShadow)::DownCast (theStructure);
  myParent = aShadow.IsNull() ? theStructure : aShadow->myParent;

  ContainsFacet = myParent->ContainsFacet;
  IsInfinite    = myParent->IsInfinite;
  myBndBox      = myParent->BoundingBox();

  Vulkan_Structure::SetTransformation (myParent->Transformation());
  myInstancedStructure = const_cast<Vulkan_Structure*> (myParent->InstancedStructure());
  myTrsfPers = myParent->TransformPersistence();

  // reuse instanced structure API
  myInstancedStructure = myParent.operator->();
}

// =======================================================================
// function : Connect
// purpose  :
// =======================================================================
void Vulkan_StructureShadow::Connect (Graphic3d_CStructure& )
{
  throw Standard_ProgramError("Error! Vulkan_StructureShadow::Connect() should not be called!");
}

// =======================================================================
// function : Disconnect
// purpose  :
// =======================================================================
void Vulkan_StructureShadow::Disconnect (Graphic3d_CStructure& )
{
  throw Standard_ProgramError("Error! Vulkan_StructureShadow::Disconnect() should not be called!");
}
