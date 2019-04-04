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

#include <Vulkan_Structure.hxx>

#include <Vulkan_Context.hxx>
#include <Vulkan_Device.hxx>
#include <Vulkan_GraphicDriver.hxx>
#include <Vulkan_Group.hxx>
#include <Vulkan_StructureShadow.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_Structure, Graphic3d_CStructure)

// =======================================================================
// function : Vulkan_Structure
// purpose  :
// =======================================================================
Vulkan_Structure::Vulkan_Structure (const Handle(Graphic3d_StructureManager)& theManager)
: Graphic3d_CStructure (theManager),
  myInstancedStructure (NULL),
  myIsMirrored         (Standard_False)
{
  updateLayerTransformation();
}

// =======================================================================
// function : ~Vulkan_Structure
// purpose  :
// =======================================================================
Vulkan_Structure::~Vulkan_Structure()
{
  /// TODO
  ///Release (Handle(OpenGl_Context)());
}

// =======================================================================
// function : SetZLayer
// purpose  :
// =======================================================================
void Vulkan_Structure::SetZLayer (const Graphic3d_ZLayerId theLayerIndex)
{
  Graphic3d_CStructure::SetZLayer (theLayerIndex);
  updateLayerTransformation();
}

// =======================================================================
// function : SetTransformation
// purpose  :
// =======================================================================
void Vulkan_Structure::SetTransformation (const Handle(Geom_Transformation)& theTrsf)
{
  myTrsf = theTrsf;
  myIsMirrored = Standard_False;
  if (!myTrsf.IsNull())
  {
    // Determinant of transform matrix less then 0 means that mirror transform applied.
    const Standard_Real aDet = myTrsf->Value(1, 1) * (myTrsf->Value (2, 2) * myTrsf->Value (3, 3) - myTrsf->Value (3, 2) * myTrsf->Value (2, 3))
                             - myTrsf->Value(1, 2) * (myTrsf->Value (2, 1) * myTrsf->Value (3, 3) - myTrsf->Value (3, 1) * myTrsf->Value (2, 3))
                             + myTrsf->Value(1, 3) * (myTrsf->Value (2, 1) * myTrsf->Value (3, 2) - myTrsf->Value (3, 1) * myTrsf->Value (2, 2));
    myIsMirrored = aDet < 0.0;
  }

  updateLayerTransformation();
}

// =======================================================================
// function : SetTransformPersistence
// purpose  :
// =======================================================================
void Vulkan_Structure::SetTransformPersistence (const Handle(Graphic3d_TransformPers)& theTrsfPers)
{
  myTrsfPers = theTrsfPers;
  updateLayerTransformation();
}

// =======================================================================
// function : updateLayerTransformation
// purpose  :
// =======================================================================
void Vulkan_Structure::updateLayerTransformation()
{
  gp_Trsf aRenderTrsf;
  if (!myTrsf.IsNull())
  {
    aRenderTrsf = myTrsf->Trsf();
  }

  const Graphic3d_ZLayerSettings& aLayer = myGraphicDriver->ZLayerSettings (myZLayer);
  if (!aLayer.OriginTransformation().IsNull()
    && myTrsfPers.IsNull())
  {
    aRenderTrsf.SetTranslationPart (aRenderTrsf.TranslationPart() - aLayer.Origin());
  }
  aRenderTrsf.GetMat4 (myRenderTrsf);
}

// =======================================================================
// function : GraphicHighlight
// purpose  :
// =======================================================================
void Vulkan_Structure::GraphicHighlight (const Handle(Graphic3d_PresentationAttributes)& theStyle)
{
  myHighlightStyle = theStyle;
  highlight = 1;
}

// =======================================================================
// function : GraphicUnhighlight
// purpose  :
// =======================================================================
void Vulkan_Structure::GraphicUnhighlight()
{
  highlight = 0;
  myHighlightStyle.Nullify();
}

// =======================================================================
// function : OnVisibilityChanged
// purpose  :
// =======================================================================
void Vulkan_Structure::OnVisibilityChanged()
{
  //
}

// =======================================================================
// function : Connect
// purpose  :
// =======================================================================
void Vulkan_Structure::Connect (Graphic3d_CStructure& theStructure)
{
  Vulkan_Structure* aStruct = static_cast<Vulkan_Structure*> (&theStructure);
  Standard_ASSERT_RAISE (myInstancedStructure == NULL || myInstancedStructure == aStruct,
                         "Error! Instanced structure is already defined");
  myInstancedStructure = aStruct;
}

// =======================================================================
// function : Disconnect
// purpose  :
// =======================================================================
void Vulkan_Structure::Disconnect (Graphic3d_CStructure& theStructure)
{
  Vulkan_Structure* aStruct = static_cast<Vulkan_Structure*> (&theStructure);
  if (myInstancedStructure == aStruct)
  {
    myInstancedStructure = NULL;
  }
}

// =======================================================================
// function : NewGroup
// purpose  :
// =======================================================================
Handle(Graphic3d_Group) Vulkan_Structure::NewGroup (const Handle(Graphic3d_Structure)& theStruct)
{
  Handle(Vulkan_Group) aGroup = new Vulkan_Group (theStruct);
  myGroups.Append (aGroup);
  return aGroup;
}

// =======================================================================
// function : RemoveGroup
// purpose  :
// =======================================================================
void Vulkan_Structure::RemoveGroup (const Handle(Graphic3d_Group)& theGroup)
{
  if (theGroup.IsNull())
  {
    return;
  }

  for (Graphic3d_SequenceOfGroup::Iterator aGroupIter (myGroups); aGroupIter.More(); aGroupIter.Next())
  {
    // Check for the given group
    if (aGroupIter.Value() == theGroup)
    {
      theGroup->Clear (Standard_False);
      myGroups.Remove (aGroupIter);
      return;
    }
  }
}

// =======================================================================
// function : Clear
// purpose  :
// =======================================================================
void Vulkan_Structure::Clear()
{
  for (Vulkan_Structure::GroupIterator aGroupIter (myGroups); aGroupIter.More(); aGroupIter.Next())
  {
    aGroupIter.ChangeValue()->Release();
  }
  myGroups.Clear();

  Is2dText       = false;
  IsForHighlight = false;
}

// =======================================================================
// function : Release
// purpose  :
// =======================================================================
void Vulkan_Structure::Release()
{
  Clear();
  myHighlightStyle.Nullify();
}

// =======================================================================
// function : ReleaseVkResources
// purpose  :
// =======================================================================
void Vulkan_Structure::ReleaseVkResources()
{
  for (Vulkan_Structure::GroupIterator aGroupIter (myGroups); aGroupIter.More(); aGroupIter.Next())
  {
    aGroupIter.ChangeValue()->Release();
  }
}

//=======================================================================
//function : ShadowLink
//purpose  :
//=======================================================================
Handle(Graphic3d_CStructure) Vulkan_Structure::ShadowLink (const Handle(Graphic3d_StructureManager)& theManager) const
{
  return new Vulkan_StructureShadow (theManager, this);
}

// =======================================================================
// function : renderGeometry
// purpose  :
// =======================================================================
void Vulkan_Structure::renderGeometry (const Handle(Vulkan_Context)& theCtx,
                                       bool& theHasClosed) const
{
  if (myInstancedStructure != NULL)
  {
    myInstancedStructure->renderGeometry (theCtx, theHasClosed);
  }

  for (Vulkan_Structure::GroupIterator aGroupIter (myGroups); aGroupIter.More(); aGroupIter.Next())
  {
    theHasClosed = theHasClosed || aGroupIter.Value()->IsClosed();
    aGroupIter.ChangeValue()->Render (theCtx);
  }
}

// =======================================================================
// function : Render
// purpose  :
// =======================================================================
void Vulkan_Structure::Render (const Handle(Vulkan_Context)& theCtx) const
{
  // Process the structure only if visible
  if (!visible)
  {
    return;
  }

  /*const Handle(Vulkan_Device)& aDevice = theCtx->Device();

  // Render named status
  if (highlight && !myHighlightStyle.IsNull() && myHighlightStyle->Method() != Aspect_TOHM_BOUNDBOX)
  {
    theWorkspace->SetHighlightStyle (myHighlightStyle);
  }

  // Apply local transformation
  aCtx->ModelWorldState.Push();
  OpenGl_Mat4& aModelWorld = aCtx->ModelWorldState.ChangeCurrent();
  aModelWorld = myRenderTrsf;

  const Standard_Boolean anOldGlNormalize = aCtx->IsGlNormalizeEnabled();
  if (!myTrsfPers.IsNull())
  {
    aCtx->WorldViewState.Push();
    OpenGl_Mat4& aWorldView = aCtx->WorldViewState.ChangeCurrent();
    myTrsfPers->Apply (theWorkspace->View()->Camera(),
                       aCtx->ProjectionState.Current(), aWorldView,
                       aCtx->VirtualViewport()[2], aCtx->VirtualViewport()[3]);
  }

  // Take into account transform persistence
  aCtx->ApplyModelViewMatrix();

  // remember aspects
  const OpenGl_Aspects* aPrevAspectFace = theWorkspace->Aspects();

  // Apply correction for mirror transform
  if (myIsMirrored)
  {
    aCtx->core11fwd->glFrontFace (GL_CW);
  }

  // Collect clipping planes of structure scope
  aCtx->ChangeClipping().SetLocalPlanes (myClipPlanes);*/

  // True if structure is fully clipped
  bool isClipped = false;
  /*bool hasDisabled = false;
  if (aCtx->Clipping().IsClippingOrCappingOn())
  {
    const Graphic3d_BndBox3d& aBBox = BoundingBox();
    if (!myClipPlanes.IsNull()
      && myClipPlanes->ToOverrideGlobal())
    {
      aCtx->ChangeClipping().DisableGlobal();
      hasDisabled = aCtx->Clipping().HasDisabled();
    }
    else if (!myTrsfPers.IsNull())
    {
      if (myTrsfPers->IsZoomOrRotate())
      {
        // Zoom/rotate persistence object lives in two worlds at the same time.
        // Global clipping planes can not be trivially applied without being converted
        // into local space of transformation persistence object.
        // As more simple alternative - just clip entire object by its anchor point defined in the world space.
        const gp_Pnt anAnchor = myTrsfPers->AnchorPoint();
        for (OpenGl_ClippingIterator aPlaneIt (aCtx->Clipping()); aPlaneIt.More() && aPlaneIt.IsGlobal(); aPlaneIt.Next())
        {
          const Handle(Graphic3d_ClipPlane)& aPlane = aPlaneIt.Value();
          if (!aPlane->IsOn())
          {
            continue;
          }

          // check for clipping
          const Graphic3d_Vec4d aCheckPnt (anAnchor.X(), anAnchor.Y(), anAnchor.Z(), 1.0);
          if (aPlane->ProbePoint (aCheckPnt) == Graphic3d_ClipState_Out)
          {
            isClipped = true;
            break;
          }
        }
      }

      aCtx->ChangeClipping().DisableGlobal();
      hasDisabled = aCtx->Clipping().HasDisabled();
    }

    // Set of clipping planes that do not intersect the structure,
    // and thus can be disabled to improve rendering performance
    if (aBBox.IsValid()
     && myTrsfPers.IsNull())
    {
      for (OpenGl_ClippingIterator aPlaneIt (aCtx->Clipping()); aPlaneIt.More(); aPlaneIt.Next())
      {
        const Handle(Graphic3d_ClipPlane)& aPlane = aPlaneIt.Value();
        if (aPlaneIt.IsDisabled())
        {
          continue;
        }

        const Graphic3d_ClipState aBoxState = aPlane->ProbeBox (aBBox);
        if (aBoxState == Graphic3d_ClipState_Out)
        {
          isClipped = true;
          break;
        }
        else if (aBoxState == Graphic3d_ClipState_In)
        {
          aCtx->ChangeClipping().SetEnabled (aPlaneIt, false);
          hasDisabled = true;
        }
      }
    }

    if ((!myClipPlanes.IsNull() && !myClipPlanes->IsEmpty())
     || hasDisabled)
    {
      // Set OCCT state uniform variables
      aCtx->ShaderManager()->UpdateClippingState();
    }
  }*/

  // Render groups
  bool hasClosedPrims = false;
  if (!isClipped)
  {
    renderGeometry (theCtx, hasClosedPrims);
  }

  // Reset correction for mirror transform
  /*if (myIsMirrored)
  {
    aCtx->core11fwd->glFrontFace (GL_CCW);
  }

  // Render capping for structure groups
  if (hasClosedPrims
   && aCtx->Clipping().IsCappingOn())
  {
    OpenGl_CappingAlgo::RenderCapping (theWorkspace, *this);
  }

  // Revert structure clippings
  if (hasDisabled)
  {
    // enable planes that were previously disabled
    aCtx->ChangeClipping().RestoreDisabled();
  }
  aCtx->ChangeClipping().SetLocalPlanes (Handle(Graphic3d_SequenceOfHClipPlane)());
  if ((!myClipPlanes.IsNull() && !myClipPlanes->IsEmpty())
    || hasDisabled)
  {
    // Set OCCT state uniform variables
    aCtx->ShaderManager()->RevertClippingState();
  }

  // Restore local transformation
  aCtx->ModelWorldState.Pop();
  aCtx->SetGlNormalizeEnabled (anOldGlNormalize);

  // Restore aspects
  theWorkspace->SetAspects (aPrevAspectFace);

  // Apply highlight box
  if (!isClipped
   && !myHighlightStyle.IsNull()
   &&  myHighlightStyle->Method() == Aspect_TOHM_BOUNDBOX)
  {
    aCtx->ApplyModelViewMatrix();
    theWorkspace->SetHighlightStyle (myHighlightStyle);
    renderBoundingBox (theWorkspace);
  }

  if (!myTrsfPers.IsNull())
  {
    aCtx->WorldViewState.Pop();
  }

  // Restore named status
  theWorkspace->SetHighlightStyle (Handle(Graphic3d_PresentationAttributes)());*/
}
