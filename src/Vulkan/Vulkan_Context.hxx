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

#ifndef _Vulkan_Context_HeaderFile
#define _Vulkan_Context_HeaderFile

#include <Graphic3d_TypeOfShadingModel.hxx>
#include <Graphic3d_TypeOfPrimitiveArray.hxx>
#include <NCollection_IndexedMap.hxx>
#include <Vulkan_Object.hxx>
#include <Vulkan_Pipeline.hxx>
#include <TCollection_AsciiString.hxx>

class Graphic3d_Camera;
class Vulkan_Aspects;
class Vulkan_CommandBuffer;
class Vulkan_CommandPool;
class Vulkan_DescriptorPool;
class Vulkan_MaterialUniformBuffer;

//! Rendering context for Vulkan.
class Vulkan_Context : public Vulkan_Object
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_Context, Vulkan_Object)
public:

  //! Constructor.
  Standard_EXPORT Vulkan_Context();

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_Context();

  //! Initialize context.
  Standard_EXPORT void Init (const Handle(Vulkan_Device)& theDevice);

  Standard_EXPORT bool ResetState (const Handle(Graphic3d_Camera)& theCamera);

  //! Return camera.
  const Handle(Graphic3d_Camera)& Camera() const { return myCamera; }

  //! Set camera.
  void SetCamera (const Handle(Graphic3d_Camera)& theCamera) { myCamera = theCamera; }

  //! Return render pass.
  const Handle(Vulkan_RenderPass)& RenderPass() const { return myRenderPass; }

  //! Set render pass.
  void SetRenderPass (const Handle(Vulkan_RenderPass)& theRenderPass) { myRenderPass = theRenderPass; }

  //! Return command pool.
  const Handle(Vulkan_CommandPool)& CommandPool() const { return myCmdPool; }

  //! Return command buffer.
  const Handle(Vulkan_CommandBuffer)& CommandBuffer() const { return myCmdBuffer; }

  //! Set command buffer.
  Standard_EXPORT void SetCommandBuffer (const Handle(Vulkan_CommandBuffer)& theBuffer);

  //! Return descriptor pool.
  const Handle(Vulkan_DescriptorPool)& DescriptorPool() const { return myDescPool; }

  //! Activate pipeline.
  Standard_EXPORT const Handle(Vulkan_Pipeline)& ActivatePipeline (const Vulkun_PipelineCfg& theCfg);

public:

  //! Return default shading model.
  Graphic3d_TypeOfShadingModel DefaultShadingModel() const { return myDefaultShadingModel; }

  //! Set default shading model.
  void SetDefaultShadingModel (Graphic3d_TypeOfShadingModel theModel) { myDefaultShadingModel = theModel; }

  //! Return active aspects.
  const Handle(Vulkan_Aspects)& ActiveAspects() const { return myActiveAspects; }

  //! Set active aspects.
  void SetActiveAspects (const Handle(Vulkan_Aspects)& theAspects) { myActiveAspects = theAspects; }

  const Handle(Vulkan_MaterialUniformBuffer)& Materials() const { return myMaterials; }

protected:

  //! Release the object.
  virtual void release() Standard_OVERRIDE;

  //! Map storing registered fonts.
  class Vulkan_PipelineMap : public NCollection_IndexedMap<Handle(Vulkan_Pipeline), Vulkan_Pipeline>
  {
  public:
    //! Empty constructor.
    Vulkan_PipelineMap() {}

    //! Try finding font with specified parameters or the closest one.
    Standard_Integer Find (const Vulkun_PipelineCfg& theCfg) const
    {
      if (IsEmpty())
      {
        return 0;
      }

      for (IndexedMapNode* aNodeIter = (IndexedMapNode* )myData1[Vulkun_PipelineCfg::HashCode (theCfg, NbBuckets())];
           aNodeIter != NULL; aNodeIter = (IndexedMapNode* )aNodeIter->Next())
      {
        const Handle(Vulkan_Pipeline)& aKey = aNodeIter->Key1();
        if (aKey->Configuration().IsEqual (theCfg))
        {
          return aNodeIter->Index();
        }
      }
      return 0;
    }
  };

protected:

  Handle(Graphic3d_Camera)      myCamera;
  Handle(Vulkan_RenderPass)     myRenderPass;
  Handle(Vulkan_CommandPool)    myCmdPool;
  Handle(Vulkan_DescriptorPool) myDescPool;
  Handle(Vulkan_CommandBuffer)  myCmdBuffer;
  Handle(Vulkan_Pipeline)       myActivePipeline;
  Vulkan_PipelineMap            myPipelineMap;

  Handle(Vulkan_MaterialUniformBuffer) myMaterials;
  Handle(Vulkan_Aspects)        myActiveAspects;
  Graphic3d_TypeOfShadingModel  myDefaultShadingModel;

};

#endif // _Vulkan_Context_HeaderFile
