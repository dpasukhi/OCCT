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

#include <Vulkan_Group.hxx>

#include <Graphic3d_ArrayOfPrimitives.hxx>
#include <Graphic3d_GroupDefinitionError.hxx>
#include <Graphic3d_Structure.hxx>
#include <Vulkan_Context.hxx>
#include <Vulkan_GraphicDriver.hxx>
#include <Vulkan_PrimitiveArray.hxx>
#include <Vulkan_Structure.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_Group, Graphic3d_Group)

namespace
{
  //! Render element if it passes the filtering procedure. This method should
  //! be used for elements which can be used in scope of rendering algorithms.
  //! E.g. elements of groups during recursive rendering.
  //! If render filter is null, pure rendering is performed.
  //! @param theWorkspace [in] the rendering workspace.
  //! @param theFilter [in] the rendering filter to check whether the element
  //! should be rendered or not.
  //! @return True if element passes the check and renders,
  /*static bool renderFiltered (const Handle(OpenGl_Workspace)& theWorkspace,
                              OpenGl_Element* theElement)
  {
    if (!theWorkspace->ShouldRender (theElement))
    {
      return false;
    }

    theElement->Render (theWorkspace);
    return true;
  }*/
}

// =======================================================================
// function : Vulkan_Group
// purpose  :
// =======================================================================
Vulkan_Group::Vulkan_Group (const Handle(Graphic3d_Structure)& theStruct)
: Graphic3d_Group (theStruct)
{
  const Vulkan_Structure* aStruct = dynamic_cast<Vulkan_Structure* >(myStructure->CStructure().get());
  if (aStruct == NULL)
  {
    throw Graphic3d_GroupDefinitionError("Vulkan_Group should be created by Vulkan_Structure!");
  }
}

// =======================================================================
// function : ~Vulkan_Group
// purpose  :
// =======================================================================
Vulkan_Group::~Vulkan_Group()
{
  Release();
}

// =======================================================================
// function : SetGroupPrimitivesAspect
// purpose  :
// =======================================================================
void Vulkan_Group::SetGroupPrimitivesAspect (const Handle(Graphic3d_Aspects)& theAspect)
{
  if (IsDeleted())
  {
    return;
  }

  Vulkan_Structure*     aStruct = (Vulkan_Structure* )myStructure->CStructure().get();
  Vulkan_GraphicDriver* aDriver = (Vulkan_GraphicDriver* )aStruct->GraphicDriver().get();
  if (myAspects.IsNull())
  {
    myAspects = new Vulkan_Aspects (aDriver->SharedContext(), theAspect);
  }
  else
  {
    myAspects->SetAspect (aDriver->SharedContext(), theAspect);
  }

  Update();
}

// =======================================================================
// function : SetPrimitivesAspect
// purpose  :
// =======================================================================
void Vulkan_Group::SetPrimitivesAspect (const Handle(Graphic3d_Aspects)& theAspect)
{
  if (myAspects.IsNull())
  {
    SetGroupPrimitivesAspect (theAspect);
    return;
  }
  else if (IsDeleted())
  {
    return;
  }

  Vulkan_Structure*     aStruct = (Vulkan_Structure* )myStructure->CStructure().get();
  Vulkan_GraphicDriver* aDriver = (Vulkan_GraphicDriver* )aStruct->GraphicDriver().get();
  Handle(Vulkan_Aspects) anAspects = new Vulkan_Aspects (aDriver->SharedContext(), theAspect);
  myElements.Append (anAspects);
  Update();
}

// =======================================================================
// function : SynchronizeAspects
// purpose  :
// =======================================================================
void Vulkan_Group::SynchronizeAspects()
{
  Vulkan_Structure*     aStruct = (Vulkan_Structure* )myStructure->CStructure().get();
  Vulkan_GraphicDriver* aDriver = (Vulkan_GraphicDriver* )aStruct->GraphicDriver().get();
  if (!myAspects.IsNull())
  {
    myAspects->SynchronizeAspects (aDriver->SharedContext());
  }
  for (Vulkan_ListOfElements::Iterator aNodeIter (myElements); aNodeIter.More(); aNodeIter.Next())
  {
    aNodeIter.Value()->SynchronizeAspects (aDriver->SharedContext());
  }
}

// =======================================================================
// function : ReplaceAspects
// purpose  :
// =======================================================================
void Vulkan_Group::ReplaceAspects (const Graphic3d_MapOfAspectsToAspects& theMap)
{
  if (theMap.IsEmpty())
  {
    return;
  }

  Vulkan_Structure*     aStruct = (Vulkan_Structure* )myStructure->CStructure().get();
  Vulkan_GraphicDriver* aDriver = (Vulkan_GraphicDriver* )aStruct->GraphicDriver().get();

  Handle(Graphic3d_Aspects) anAspect;
  if (!myAspects.IsNull()
    && theMap.Find (myAspects->Aspect(), anAspect))
  {
    myAspects->SetAspect (aDriver->SharedContext(), anAspect);
  }
  for (Vulkan_ListOfElements::Iterator aNodeIter (myElements); aNodeIter.More(); aNodeIter.Next())
  {
    Vulkan_Aspects* aVkAspects = dynamic_cast<Vulkan_Aspects*> (aNodeIter.Value().get());
    if (aVkAspects != NULL
     && theMap.Find (aVkAspects->Aspect(), anAspect))
    {
      aVkAspects->SetAspect (aDriver->SharedContext(), anAspect);
    }
  }
}

// =======================================================================
// function : AddPrimitiveArray
// purpose  :
// =======================================================================
void Vulkan_Group::AddPrimitiveArray (const Graphic3d_TypeOfPrimitiveArray theType,
                                      const Handle(Graphic3d_IndexBuffer)& theIndices,
                                      const Handle(Graphic3d_Buffer)&      theAttribs,
                                      const Handle(Graphic3d_BoundBuffer)& theBounds,
                                      const Standard_Boolean               theToEvalMinMax)
{
  if (IsDeleted()
   || theAttribs.IsNull())
  {
    return;
  }

  Handle(Vulkan_PrimitiveArray) aPrimArray = new Vulkan_PrimitiveArray (theType, theIndices, theAttribs, theBounds);
  myElements.Append (aPrimArray);

  Graphic3d_Group::AddPrimitiveArray (theType, theIndices, theAttribs, theBounds, theToEvalMinMax);
}

// =======================================================================
// function : Text
// purpose  :
// =======================================================================
void Vulkan_Group::Text (const Standard_CString                  theTextUtf,
                         const Graphic3d_Vertex&                 thePoint,
                         const Standard_Real                     theHeight,
                         const Standard_Real                     theAngle,
                         const Graphic3d_TextPath                theTp,
                         const Graphic3d_HorizontalTextAlignment theHta,
                         const Graphic3d_VerticalTextAlignment   theVta,
                         const Standard_Boolean                  theToEvalMinMax)
{
  if (IsDeleted())
  {
    return;
  }

  Graphic3d_Group::Text (theTextUtf, thePoint, theHeight, theAngle,
                         theTp, theHta, theVta, theToEvalMinMax);
}

// =======================================================================
// function : Text
// purpose  :
// =======================================================================
void Vulkan_Group::Text (const Standard_CString                  theTextUtf,
                         const gp_Ax2&                           theOrientation,
                         const Standard_Real                     theHeight,
                         const Standard_Real                     theAngle,
                         const Graphic3d_TextPath                theTp,
                         const Graphic3d_HorizontalTextAlignment theHTA,
                         const Graphic3d_VerticalTextAlignment   theVTA,
                         const Standard_Boolean                  theToEvalMinMax,
                         const Standard_Boolean                  theHasOwnAnchor)
{
  if (IsDeleted())
  {
    return;
  }

  Graphic3d_Group::Text (theTextUtf,
                         theOrientation,
                         theHeight,
                         theAngle,
                         theTp,
                         theHTA,
                         theVTA,
                         theToEvalMinMax,
                         theHasOwnAnchor);
}

// =======================================================================
// function : SetFlippingOptions
// purpose  :
// =======================================================================
void Vulkan_Group::SetFlippingOptions (const Standard_Boolean ,
                                       const gp_Ax2& )
{
  //
}

// =======================================================================
// function : SetStencilTestOptions
// purpose  :
// =======================================================================
void Vulkan_Group::SetStencilTestOptions (const Standard_Boolean )
{
  //
}

// =======================================================================
// function : Render
// purpose  :
// =======================================================================
void Vulkan_Group::Render (const Handle(Vulkan_Context)& theCtx)
{
  // Setup aspects
  /*theWorkspace->SetAllowFaceCulling (myIsClosed
                                 && !theWorkspace->GetGlContext()->Clipping().IsClippingOrCappingOn());
  const OpenGl_Aspects* aBackAspects = theWorkspace->Aspects();
  const bool isAspectSet = myAspects != NULL && renderFiltered (theWorkspace, myAspects);*/
  theCtx->SetActiveAspects (myAspects);

  // Render group elements
  for (Vulkan_ListOfElements::Iterator aNodeIter (myElements); aNodeIter.More(); aNodeIter.Next())
  {
    aNodeIter.Value()->Render (theCtx);
    ///renderFiltered (theWorkspace, aNodeIter->elem);
  }

  // Restore aspects
  /*if (isAspectSet)
    theWorkspace->SetAspects (aBackAspects);*/
  theCtx->SetActiveAspects (Handle(Vulkan_Aspects)()); //
}

// =======================================================================
// function : Clear
// purpose  :
// =======================================================================
void Vulkan_Group::Clear (const Standard_Boolean theToUpdateStructureMgr)
{
  if (IsDeleted())
  {
    return;
  }

  Release();
  Graphic3d_Group::Clear (theToUpdateStructureMgr);
}

// =======================================================================
// function : Release
// purpose  :
// =======================================================================
void Vulkan_Group::Release()
{
  for (Vulkan_ListOfElements::Iterator aNodeIter (myElements); aNodeIter.More(); aNodeIter.Next())
  {
    aNodeIter.ChangeValue()->Release();
  }
  myElements.Clear();

  if (!myAspects.IsNull())
  {
    myAspects->Release();
    myAspects.Nullify();
  }
}
