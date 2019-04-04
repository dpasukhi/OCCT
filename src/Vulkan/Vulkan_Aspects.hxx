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

#ifndef _Vulkan_Aspects_HeaderFile
#define _Vulkan_Aspects_HeaderFile

#include <Vulkan_Element.hxx>
#include <Graphic3d_Aspects.hxx>
#include <Graphic3d_BSDF.hxx>

class Vulkan_MaterialUniformBuffer;

//! The element holding Graphic3d_Aspects.
class Vulkan_Aspects : public Vulkan_Element
{
public:

  //! Empty constructor.
  Standard_EXPORT Vulkan_Aspects();

  //! Create and assign parameters.
  Standard_EXPORT Vulkan_Aspects (const Handle(Vulkan_Context)& theCtx,
                                  const Handle(Graphic3d_Aspects)& theAspect);

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_Aspects();

  //! Return aspect.
  const Handle(Graphic3d_Aspects)& Aspect() const { return myAspect; }

  //! Assign parameters.
  Standard_EXPORT void SetAspect (const Handle(Vulkan_Context)& theCtx,
                                  const Handle(Graphic3d_Aspects)& theAspect);

  //! Returns Shading Model.
  Graphic3d_TypeOfShadingModel ShadingModel() const { return myShadingModel; }

  //! Return material index.
  Standard_Integer MaterialIndex() const { return myMaterialIndex; }

  //! Release GPU resources.
  Standard_EXPORT virtual void Release() Standard_OVERRIDE;

  //! Perform rendering.
  Standard_EXPORT virtual void Render (const Handle(Vulkan_Context)& theCtx) Standard_OVERRIDE;

  //! Update presentation aspects parameters after their modification.
  virtual void SynchronizeAspects (const Handle(Vulkan_Context)& theCtx) Standard_OVERRIDE { SetAspect (theCtx, myAspect); }

protected:

  Handle(Graphic3d_Aspects)    myAspect;
  Handle(Vulkan_MaterialUniformBuffer) myMaterialsUbo;
  Graphic3d_TypeOfShadingModel myShadingModel;
  Standard_Integer             myMaterialIndex;

};

#endif // _Vulkan_Aspects_HeaderFile
