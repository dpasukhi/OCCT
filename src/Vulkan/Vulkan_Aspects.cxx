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

#include <Vulkan_Aspects.hxx>

#include <Graphic3d_MaterialAspect.hxx>
#include <Vulkan_Context.hxx>
#include <Vulkan_MaterialUniformBuffer.hxx>

namespace
{
  //! Initialize default material in this way for backward compatibility.
  inline Graphic3d_MaterialAspect initDefaultMaterial()
  {
    Graphic3d_MaterialAspect aMat;
    aMat.SetMaterialType (Graphic3d_MATERIAL_ASPECT);
    aMat.SetAmbient  (0.2f);
    aMat.SetDiffuse  (0.8f);
    aMat.SetSpecular (0.1f);
    aMat.SetEmissive (0.0f);
    aMat.SetAmbientColor (Quantity_NOC_WHITE);
    aMat.SetDiffuseColor (Quantity_NOC_WHITE);
    aMat.SetEmissiveColor(Quantity_NOC_WHITE);
    aMat.SetSpecularColor(Quantity_NOC_WHITE);
    aMat.SetShininess (10.0f / 128.0f);
    aMat.SetRefractionIndex (1.0f);
    return aMat;
  }

  static const Graphic3d_MaterialAspect THE_DEFAULT_MATERIAL = initDefaultMaterial();
}

// =======================================================================
// function : Vulkan_Aspects
// purpose  :
// =======================================================================
Vulkan_Aspects::Vulkan_Aspects()
: myAspect (new Graphic3d_Aspects()),
  myShadingModel (Graphic3d_TOSM_UNLIT),
  myMaterialIndex (0)
{
  myAspect->SetInteriorStyle (Aspect_IS_SOLID);
  myAspect->SetInteriorColor (Quantity_NOC_WHITE);
  myAspect->SetEdgeColor (Quantity_NOC_WHITE);
  myAspect->SetFrontMaterial (THE_DEFAULT_MATERIAL);
  myAspect->SetBackMaterial (THE_DEFAULT_MATERIAL);
  myAspect->SetShadingModel (myShadingModel);
  myAspect->SetHatchStyle (Handle(Graphic3d_HatchStyle)());
}

// =======================================================================
// function : Vulkan_Aspects
// purpose  :
// =======================================================================
Vulkan_Aspects::Vulkan_Aspects (const Handle(Vulkan_Context)& theCtx,
                                const Handle(Graphic3d_Aspects)& theAspect)
: myShadingModel (Graphic3d_TOSM_DEFAULT),
  myMaterialIndex (-1)
{
  SetAspect (theCtx, theAspect);
}

// =======================================================================
// function : ~Vulkan_Aspects
// purpose  :
// =======================================================================
Vulkan_Aspects::~Vulkan_Aspects()
{
  if (!myMaterialsUbo.IsNull())
  {
    myMaterialsUbo->ReleaseMaterial (myMaterialIndex);
  }
}

// =======================================================================
// function : SetAspect
// purpose  :
// =======================================================================
void Vulkan_Aspects::SetAspect (const Handle(Vulkan_Context)& theCtx,
                                const Handle(Graphic3d_Aspects)& theAspect)
{
  myAspect = theAspect;
  myMaterialsUbo  = theCtx->Materials();
  myMaterialIndex = myMaterialsUbo->AddMaterial (theAspect->ColorRGBA(), myMaterialIndex);

  const Graphic3d_MaterialAspect& aMat = theAspect->FrontMaterial();
  myShadingModel = theAspect->ShadingModel() != Graphic3d_TOSM_UNLIT
                && (aMat.ReflectionMode (Graphic3d_TOR_AMBIENT)
                 || aMat.ReflectionMode (Graphic3d_TOR_DIFFUSE)
                 || aMat.ReflectionMode (Graphic3d_TOR_SPECULAR)
                 || aMat.ReflectionMode (Graphic3d_TOR_EMISSION))
                 ? theAspect->ShadingModel()
                 : Graphic3d_TOSM_UNLIT;
}

// =======================================================================
// function : Release
// purpose  :
// =======================================================================
void Vulkan_Aspects::Release()
{
  //
}

// =======================================================================
// function : Render
// purpose  :
// =======================================================================
void Vulkan_Aspects::Render (const Handle(Vulkan_Context)& theCtx)
{
  theCtx->SetActiveAspects (this);
}
