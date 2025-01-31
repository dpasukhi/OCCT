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

#ifndef _Vulkan_Pipeline_HeaderFile
#define _Vulkan_Pipeline_HeaderFile

#include <Graphic3d_TypeOfPrimitiveArray.hxx>
#include <Graphic3d_TypeOfShadingModel.hxx>
#include <Graphic3d_Vec2.hxx>
#include <Graphic3d_Buffer.hxx>
#include <Vulkan_Object.hxx>

#include <Standard_HashUtils.hxx>

class Vulkan_PipelineCache;
class Vulkan_PipelineLayout;
class Vulkan_RenderPass;
class Vulkan_Shader;

//! Close to Graphic3d_Attribute
struct Vulkun_VertexAttribute
{
  Graphic3d_TypeOfAttribute
    Location; //!< attribute identifier in vertex shader, 0 is reserved for vertex position
  Graphic3d_TypeOfData DataType; //!< vec2,vec3,vec4,vec4ub
  uint32_t             Offset;   //!< byte offset to the data within vertex buffer

  Vulkun_VertexAttribute()
      : Location(Graphic3d_TOA_POS),
        DataType(Graphic3d_TOD_VEC3),
        Offset(0)
  {
  }

  //! Matching two instances.
  bool IsEqual(const Vulkun_VertexAttribute& theOther) const
  {
    return Location == theOther.Location && DataType == theOther.DataType
           && Offset == theOther.Offset;
  }
};

struct Vulkun_PipelineCfg
{
  Graphic3d_TypeOfPrimitiveArray PrimType;
  Graphic3d_TypeOfShadingModel   ShadingModel;
  Vulkun_VertexAttribute         Attributes[5];
  Standard_Integer               NbAttributes;
  uint32_t                       Stride;

  Vulkun_PipelineCfg()
      : PrimType(Graphic3d_TOPA_UNDEFINED),
        ShadingModel(Graphic3d_TOSM_UNLIT),
        NbAttributes(0),
        Stride(0)
  {
  }

public:
  static const Standard_Integer THE_MAX_NB_ATTRIBUTES = 5;

  //! Matching two instances, for Map interface.
  bool IsEqual(const Vulkun_PipelineCfg& theOther) const
  {
    if (PrimType != theOther.PrimType || NbAttributes != theOther.NbAttributes
        || Stride != theOther.Stride)
    {
      return false;
    }
    for (Standard_Integer anAttribIter = 0; anAttribIter < NbAttributes; ++anAttribIter)
    {
      if (!Attributes[anAttribIter].IsEqual(theOther.Attributes[anAttribIter]))
      {
        return false;
      }
    }
    return true;
  }
};

namespace std
{
template <>
struct hash<Vulkun_PipelineCfg>
{
  size_t operator()(const Vulkun_PipelineCfg& thePipelineCfg) const
  {
    // Combine two int values into a single hash value.
    size_t aCombination[3]{std::hash<Graphic3d_TypeOfPrimitiveArray>{}(thePipelineCfg.PrimType),
                           std::hash<Graphic3d_TypeOfShadingModel>{}(thePipelineCfg.ShadingModel),
                           std::hash<Standard_Integer>{}(thePipelineCfg.NbAttributes)};
    return opencascade::hashBytes(aCombination, sizeof(aCombination));
  }
};

//! This class defines an Vulkan Pipeline.
class Vulkan_Pipeline : public Vulkan_Object
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_Pipeline, Vulkan_Object)
public:
  //! Constructor.
  Standard_EXPORT Vulkan_Pipeline();

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_Pipeline();

  //! Return object.
  VkPipeline Pipeline() const { return myVkPipeline; }

  //! Return pipeline layout.
  const Handle(Vulkan_PipelineLayout)& PipelineLayout() const { return myPipelineLayout; }

  //! Create the object, @sa vkCreateGraphicsPipelines().
  Standard_EXPORT bool Create(const Handle(Vulkan_Device)&         theDevice,
                              const Handle(Vulkan_RenderPass)&     theRenderPass,
                              const Handle(Vulkan_PipelineLayout)& theLayout,
                              const Handle(Vulkan_Shader)&         theShaderVert,
                              const Handle(Vulkan_Shader)&         theShaderFrag,
                              const Graphic3d_Vec2u&               theViewport,
                              const Vulkun_PipelineCfg&            theCfg);

  //! Return primitive array type.
  const Vulkun_PipelineCfg& Configuration() const { return myCfg; }

  //! Equality check.
  bool IsEqual(const Handle(Vulkan_Pipeline)& theOther) const
  {
    return !theOther.IsNull() && (theOther == this || myCfg.IsEqual(theOther->myCfg));
  }

public:
  //! Matching two instances, for Map interface.
  static bool IsEqual(const Handle(Vulkan_Pipeline)& thePipeline1,
                      const Handle(Vulkan_Pipeline)& thePipeline2)
  {
    return thePipeline1->IsEqual(thePipeline2);
  }

protected:
  //! Release the object.
  virtual void release() Standard_OVERRIDE { releasePipeline(); }

  //! Release the object, @sa vkDestroyPipeline().
  Standard_EXPORT void releasePipeline();

protected:
  Handle(Vulkan_Shader)         myShaderVert;
  Handle(Vulkan_Shader)         myShaderFrag;
  Handle(Vulkan_PipelineCache)  myPipelineCache;
  Handle(Vulkan_PipelineLayout) myPipelineLayout;
  VkPipeline                    myVkPipeline;
  Vulkun_PipelineCfg            myCfg;
};

namespace std
{
template <>
struct hash<Handle(Vulkan_Pipeline)>
{
  size_t operator()(const Handle(Vulkan_Pipeline)& thePipeline) const
  {
    if (thePipeline.IsNull())
    {
      return 1;
    }
    return std::hash<Vulkun_PipelineCfg>{}(thePipeline->Configuration());
  }
};

template <>
struct equal_to<Handle(Vulkan_Pipeline)>
{
  bool operator()(const Handle(Vulkan_Pipeline)& thePipeline1,
                  const Handle(Vulkan_Pipeline)& thePipeline2) const
  {
    return Vulkan_Pipeline::IsEqual(thePipeline1, thePipeline2);
  }
};
} // namespace std

#endif // _Vulkan_Pipeline_HeaderFile
