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

#include <Vulkan_Pipeline.hxx>

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_Device.hxx>
#include <Vulkan_PipelineCache.hxx>
#include <Vulkan_PipelineLayout.hxx>
#include <Vulkan_RenderPass.hxx>
#include <Vulkan_Shader.hxx>

#include <vulkan/vulkan.h>

#include <vector>

namespace
{
  //! Convert Graphic3d_TypeOfPrimitiveArray enumeration to VkPrimitiveTopology.
  VkPrimitiveTopology primType2VkTopology (Graphic3d_TypeOfPrimitiveArray thePrimType)
  {
    switch (thePrimType)
    {
      case Graphic3d_TOPA_POINTS:         return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
      case Graphic3d_TOPA_SEGMENTS:       return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
      case Graphic3d_TOPA_POLYLINES:      return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
      case Graphic3d_TOPA_TRIANGLES:      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      case Graphic3d_TOPA_TRIANGLESTRIPS: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
      case Graphic3d_TOPA_TRIANGLEFANS:   return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
      //
      case Graphic3d_TOPA_LINES_ADJACENCY:          return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
      case Graphic3d_TOPA_LINE_STRIP_ADJACENCY:     return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
      case Graphic3d_TOPA_TRIANGLES_ADJACENCY:      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
      case Graphic3d_TOPA_TRIANGLE_STRIP_ADJACENCY: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
      //
      case Graphic3d_TOPA_QUADRANGLES:
      case Graphic3d_TOPA_QUADRANGLESTRIPS:
      case Graphic3d_TOPA_POLYGONS:
      case Graphic3d_TOPA_UNDEFINED:
        return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    }
    return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
  }

  static VkFormat vertAttrib2VkFormat (Graphic3d_TypeOfData theType)
  {
    switch (theType)
    {
      case Graphic3d_TOD_USHORT: return VK_FORMAT_R16_UINT;
      case Graphic3d_TOD_UINT:   return VK_FORMAT_R32_UINT;
      case Graphic3d_TOD_VEC2:   return VK_FORMAT_R32G32_SFLOAT;
      case Graphic3d_TOD_VEC3:   return VK_FORMAT_R32G32B32_SFLOAT;
      case Graphic3d_TOD_VEC4:   return VK_FORMAT_R32G32B32A32_SFLOAT;
      case Graphic3d_TOD_VEC4UB: return VK_FORMAT_B8G8R8A8_UNORM;
      case Graphic3d_TOD_FLOAT:  return VK_FORMAT_R32_SFLOAT;
    }
    return VK_FORMAT_UNDEFINED;
  }
}

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_Pipeline, Vulkan_Object)

// =======================================================================
// function : Vulkan_Pipeline
// purpose  :
// =======================================================================
Vulkan_Pipeline::Vulkan_Pipeline()
: myVkPipeline (NULL)
{
  //
}

// =======================================================================
// function : ~Vulkan_Pipeline
// purpose  :
// =======================================================================
Vulkan_Pipeline::~Vulkan_Pipeline()
{
  releasePipeline();
}

// =======================================================================
// function : releasePipeline
// purpose  :
// =======================================================================
void Vulkan_Pipeline::releasePipeline()
{
  if (myVkPipeline != NULL)
  {
    Vulkan_AssertOnRelease("Vulkan_Pipeline");
    vkDestroyPipeline (myDevice->Device(), myVkPipeline, myDevice->HostAllocator());
    myVkPipeline = NULL;
  }
  myPipelineLayout.Nullify();
  myPipelineCache.Nullify();
  myShaderVert.Nullify();
  myShaderFrag.Nullify();
  myDevice.Nullify();
}

// =======================================================================
// function : Create
// purpose  :
// =======================================================================
bool Vulkan_Pipeline::Create (const Handle(Vulkan_Device)& theDevice,
                              const Handle(Vulkan_RenderPass)& theRenderPass,
                              const Handle(Vulkan_PipelineLayout)& theLayout,
                              const Handle(Vulkan_Shader)& theShaderVert,
                              const Handle(Vulkan_Shader)& theShaderFrag,
                              const Graphic3d_Vec2u& theViewport,
                              const Vulkun_PipelineCfg& theCfg)
{
  Release();
  if (theDevice.IsNull()
   || theDevice->Device() == NULL
   || theShaderVert.IsNull()
   || theShaderVert->Shader() == NULL
   || theShaderFrag.IsNull()
   || theShaderFrag->Shader() == NULL)
  {
    return false;
  }

  myDevice = theDevice;
  myShaderVert = theShaderVert;
  myShaderFrag = theShaderFrag;
  myPipelineLayout = theLayout;
  myCfg = theCfg;
  /*if (myPipelineLayout.IsNull())
  {
    myPipelineLayout = new Vulkan_PipelineLayout();
    if (!myPipelineLayout->Create (theDevice, NULL))
    {
      return false;
    }
  }*/
  if (myPipelineCache.IsNull())
  {
    myPipelineCache = new Vulkan_PipelineCache();
    if (!myPipelineCache->Create (theDevice))
    {
      Release();
      return false;
    }
  }

  VkPipelineShaderStageCreateInfo aShaderStages[2] = {};
  {
    VkPipelineShaderStageCreateInfo& aVertStage = aShaderStages[0];
    aVertStage.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    aVertStage.pNext	= NULL;
    aVertStage.flags	= 0;
    aVertStage.stage	= VK_SHADER_STAGE_VERTEX_BIT;
    aVertStage.module	=  theShaderVert->Shader();
    aVertStage.pName	= "main";
    aVertStage.pSpecializationInfo = NULL;
  }
  {
    VkPipelineShaderStageCreateInfo& aFragStage = aShaderStages[1];
    aFragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    aFragStage.pNext = NULL;
    aFragStage.flags = 0;
    aFragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    aFragStage.module = theShaderFrag->Shader();
    aFragStage.pName = "main";
    aFragStage.pSpecializationInfo = NULL;
  }

  VkVertexInputBindingDescription aVertexInputBinding;
  aVertexInputBinding.binding   = 0;
  aVertexInputBinding.stride    = myCfg.Stride;
  aVertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  std::vector<VkVertexInputAttributeDescription> aVertexInputAttributes (myCfg.NbAttributes);
  for (Standard_Integer anAttribIter = 0; anAttribIter < myCfg.NbAttributes; ++anAttribIter)
  {
    const Vulkun_VertexAttribute& anAttrib = myCfg.Attributes[anAttribIter];
    VkVertexInputAttributeDescription& anInAttrib = aVertexInputAttributes[anAttribIter];
    anInAttrib.location = anAttrib.Location;
    anInAttrib.binding = 0;
    anInAttrib.format = vertAttrib2VkFormat (anAttrib.DataType);
    anInAttrib.offset = anAttrib.Offset;
  }

  VkPipelineVertexInputStateCreateInfo aVertexInput;
  aVertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  aVertexInput.pNext = NULL;
  aVertexInput.flags = 0;
  aVertexInput.vertexBindingDescriptionCount   = 1;
  aVertexInput.pVertexBindingDescriptions      = &aVertexInputBinding;
  aVertexInput.vertexAttributeDescriptionCount = (uint32_t )aVertexInputAttributes.size();
  aVertexInput.pVertexAttributeDescriptions    = aVertexInputAttributes.data();

  VkPipelineInputAssemblyStateCreateInfo anInputAssembly;
  anInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  anInputAssembly.pNext = NULL;
  anInputAssembly.flags = 0;
  anInputAssembly.topology = primType2VkTopology (myCfg.PrimType);
  anInputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport aViewport = {};
  aViewport.x = 0.0f;
  aViewport.y = (float )theViewport.y();
  aViewport.width  =  (float )theViewport.x();
  aViewport.height = -(float )theViewport.y();
  aViewport.minDepth = 0.0f;
  aViewport.maxDepth = 1.0f;
  VkRect2D aScissor = {};
  aScissor.offset.x = 0;
  aScissor.offset.y = 0;
  aScissor.extent.width  = theViewport.x();
  aScissor.extent.height = theViewport.y();

  VkPipelineViewportStateCreateInfo aViewportInfo;
  aViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  aViewportInfo.pNext = NULL;
  aViewportInfo.flags = 0;
  aViewportInfo.viewportCount = 1;
  aViewportInfo.pViewports = &aViewport;
  aViewportInfo.scissorCount = 1;
  aViewportInfo.pScissors = &aScissor;

  VkPipelineRasterizationStateCreateInfo aRaster;
  aRaster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  aRaster.pNext = NULL;
  aRaster.flags = 0;
  aRaster.depthClampEnable = VK_FALSE;
  aRaster.rasterizerDiscardEnable = VK_FALSE;
  aRaster.polygonMode = VK_POLYGON_MODE_FILL;
  aRaster.cullMode = VK_CULL_MODE_BACK_BIT; ///VK_CULL_MODE_NONE;
  aRaster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  ///aRaster.frontFace = VK_FRONT_FACE_CLOCKWISE;
  aRaster.depthBiasEnable = VK_FALSE;
  aRaster.depthBiasConstantFactor = 0.0f;
  aRaster.depthBiasClamp = 0.0f;
  aRaster.depthBiasSlopeFactor = 0.0f;
  aRaster.lineWidth = 1.0f;

  VkPipelineMultisampleStateCreateInfo aMultisample;
  aMultisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  aMultisample.pNext = NULL;
  aMultisample.flags = 0;
  aMultisample.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
  aMultisample.sampleShadingEnable = VK_FALSE;
  aMultisample.minSampleShading = 0;
  aMultisample.pSampleMask = NULL;
  aMultisample.alphaToCoverageEnable = VK_FALSE;
  aMultisample.alphaToOneEnable = VK_FALSE;

  VkStencilOpState aNoOpStencil = {};
  aNoOpStencil.failOp      = VK_STENCIL_OP_KEEP;
  aNoOpStencil.passOp      = VK_STENCIL_OP_KEEP;
  aNoOpStencil.depthFailOp = VK_STENCIL_OP_KEEP;
  aNoOpStencil.compareOp   = VK_COMPARE_OP_ALWAYS;
  aNoOpStencil.compareMask = 0;
  aNoOpStencil.writeMask   = 0;
  aNoOpStencil.reference   = 0;

  VkPipelineDepthStencilStateCreateInfo aDepthStencil;
  aDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  aDepthStencil.pNext = NULL;
  aDepthStencil.flags = 0;
  aDepthStencil.depthTestEnable  = VK_TRUE;
  aDepthStencil.depthWriteEnable = VK_TRUE;
  aDepthStencil.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;
  aDepthStencil.depthBoundsTestEnable = VK_FALSE;
  aDepthStencil.stencilTestEnable = VK_FALSE;
  aDepthStencil.front = aNoOpStencil;
  aDepthStencil.back  = aNoOpStencil;
  aDepthStencil.minDepthBounds = 0.0f;
  aDepthStencil.maxDepthBounds = 0.0f;

  VkPipelineColorBlendAttachmentState aBlendAttachments;
  aBlendAttachments.blendEnable         = VK_FALSE;
  aBlendAttachments.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  aBlendAttachments.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
  aBlendAttachments.colorBlendOp        = VK_BLEND_OP_ADD;
  aBlendAttachments.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  aBlendAttachments.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  aBlendAttachments.alphaBlendOp        = VK_BLEND_OP_ADD;
  aBlendAttachments.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

  VkPipelineColorBlendStateCreateInfo aBlending;
  aBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  aBlending.pNext = NULL;
  aBlending.flags = 0;
  aBlending.logicOpEnable = VK_FALSE;
  aBlending.logicOp = VK_LOGIC_OP_CLEAR;
  aBlending.attachmentCount = 1;
  aBlending.pAttachments = &aBlendAttachments;
  aBlending.blendConstants;
  aBlending.blendConstants[0] = 1.0f;
  aBlending.blendConstants[1] = 1.0f;
  aBlending.blendConstants[2] = 1.0f;
  aBlending.blendConstants[3] = 1.0f;

  VkGraphicsPipelineCreateInfo aPipeInfo;
  aPipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  aPipeInfo.pNext = NULL;
  aPipeInfo.flags = 0;
  aPipeInfo.stageCount = 2;
  aPipeInfo.pStages    = aShaderStages;
  aPipeInfo.pVertexInputState   = &aVertexInput;
  aPipeInfo.pInputAssemblyState = &anInputAssembly;
  aPipeInfo.pTessellationState  = VK_NULL_HANDLE;
  aPipeInfo.pViewportState      = &aViewportInfo;
  aPipeInfo.pRasterizationState = &aRaster;
  aPipeInfo.pMultisampleState   = &aMultisample;
  aPipeInfo.pDepthStencilState  = &aDepthStencil;
  aPipeInfo.pColorBlendState    = &aBlending;
  aPipeInfo.pDynamicState       = NULL;
  aPipeInfo.layout     = myPipelineLayout->PipelineLayout();
  aPipeInfo.renderPass = theRenderPass->RenderPass();
  aPipeInfo.subpass    = 0;
  aPipeInfo.basePipelineIndex = 0;
  aPipeInfo.basePipelineHandle = VK_NULL_HANDLE;

  VkResult aRes = vkCreateGraphicsPipelines (theDevice->Device(),
                                             !myPipelineCache.IsNull() ? myPipelineCache->PipelineCache() : NULL,
                                             1, &aPipeInfo,
                                             theDevice->HostAllocator(), &myVkPipeline);
  if (aRes != VK_SUCCESS)
  {
    logFailureAndRelease ("failed to create pipeline", aRes);
    return false;
  }

  return true;
}
