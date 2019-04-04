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

#include <Vulkan_Context.hxx>

#include <Graphic3d_Camera.hxx>
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_Aspects.hxx>
#include <Vulkan_CommandBuffer.hxx>
#include <Vulkan_CommandPool.hxx>
#include <Vulkan_DescriptorPool.hxx>
#include <Vulkan_DescriptorSetLayout.hxx>
#include <Vulkan_Device.hxx>
#include <Vulkan_MaterialUniformBuffer.hxx>
#include <Vulkan_PipelineLayout.hxx>
#include <Vulkan_Shader.hxx>

#include <vulkan/vulkan.h>

#include <vector>

#include "Vulkan_ShaderUnlit_vs_spv.pxx"
#include "Vulkan_ShaderUnlit_fs_spv.pxx"
#include "Vulkan_ShaderPhong_vs_spv.pxx"
#include "Vulkan_ShaderPhong_fs_spv.pxx"
#include "Vulkan_ShaderFlat_vs_spv.pxx"
#include "Vulkan_ShaderFlat_fs_spv.pxx"

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_Context, Vulkan_Object)

// =======================================================================
// function : Vulkan_Context
// purpose  :
// =======================================================================
Vulkan_Context::Vulkan_Context()
: myCmdBuffer (NULL),
  myMaterials (new Vulkan_MaterialUniformBuffer()),
  myDefaultShadingModel (Graphic3d_TOSM_FRAGMENT)
{
  //
}

// =======================================================================
// function : ~Vulkan_Context
// purpose  :
// =======================================================================
Vulkan_Context::~Vulkan_Context()
{
  //
}

// =======================================================================
// function : release
// purpose  :
// =======================================================================
void Vulkan_Context::release()
{
  myCmdBuffer.Nullify();
  myDescPool.Nullify();
  myCmdPool.Nullify();
  myDevice.Nullify();
  myMaterials->Release();
}

// =======================================================================
// function : Init
// purpose  :
// =======================================================================
void Vulkan_Context::Init (const Handle(Vulkan_Device)& theDevice)
{
  myDevice = theDevice;
  myCmdPool = new Vulkan_CommandPool();
  myCmdPool->Create (theDevice);
  myDescPool = new Vulkan_DescriptorPool();
  myDescPool->Create (theDevice);
  myMaterials->SetAlignment (theDevice);
}

// =======================================================================
// function : SetCommandBuffer
// purpose  :
// =======================================================================
void Vulkan_Context::SetCommandBuffer (const Handle(Vulkan_CommandBuffer)& theBuffer)
{
  myCmdBuffer = theBuffer;
}

// =======================================================================
// function : ResetState
// purpose  :
// =======================================================================
bool Vulkan_Context::ResetState (const Handle(Graphic3d_Camera)& theCamera)
{
  myCamera = theCamera;
  myActivePipeline.Nullify();
  if (!myCmdPool->ResetPool()
   || !myDescPool->ResetPool()
   || !myMaterials->Init (myDevice))
  {
    return false;
  }
  return true;
}

// =======================================================================
// function : ActivatePipeline
// purpose  :
// =======================================================================
const Handle(Vulkan_Pipeline)& Vulkan_Context::ActivatePipeline (const Vulkun_PipelineCfg& theCfg)
{
  static Handle(Vulkan_PipelineLayout) aPipeLayout;
  static Handle(Vulkan_DescriptorSetLayout) aDescSetLayout = new Vulkan_DescriptorSetLayout();
  static Handle(Vulkan_UniformBuffer) aUboMatrixes;
  if (aPipeLayout.IsNull())
  {
    aDescSetLayout->Create (myDevice);

    aPipeLayout = new Vulkan_PipelineLayout();
    aPipeLayout->Create (myDevice, aDescSetLayout);

    aUboMatrixes = new Vulkan_UniformBuffer();
  }

  Standard_Integer anIndex = myPipelineMap.Find (theCfg);
  if (anIndex == 0)
  {
    Handle(Vulkan_Shader) aShaderVert, aShaderFrag;
    aShaderVert = new Vulkan_Shader();
    aShaderFrag = new Vulkan_Shader();
    if (theCfg.ShadingModel == Graphic3d_TOSM_VERTEX
     || theCfg.ShadingModel == Graphic3d_TOSM_FRAGMENT)
    {
      aShaderVert->Create (myDevice, Vulkan_ShaderPhong_vs_spv, sizeof(Vulkan_ShaderPhong_vs_spv));
      aShaderFrag->Create (myDevice, Vulkan_ShaderPhong_fs_spv, sizeof(Vulkan_ShaderPhong_fs_spv));
    }
    else if (theCfg.ShadingModel == Graphic3d_TOSM_FACET)
    {
      aShaderVert->Create (myDevice, Vulkan_ShaderFlat_vs_spv, sizeof(Vulkan_ShaderFlat_vs_spv));
      aShaderFrag->Create (myDevice, Vulkan_ShaderFlat_fs_spv, sizeof(Vulkan_ShaderFlat_fs_spv));
    }
    else
    {
      aShaderVert->Create (myDevice, Vulkan_ShaderUnlit_vs_spv, sizeof(Vulkan_ShaderUnlit_vs_spv));
      aShaderFrag->Create (myDevice, Vulkan_ShaderUnlit_fs_spv, sizeof(Vulkan_ShaderUnlit_fs_spv));
    }

    Handle(Vulkan_Pipeline) aPipeline = new Vulkan_Pipeline();
    aPipeline->Create (myDevice, myRenderPass, aPipeLayout, aShaderVert, aShaderFrag, Graphic3d_Vec2u (400, 400), theCfg);
    anIndex = myPipelineMap.Add (aPipeline);
  }

  const Handle(Vulkan_Pipeline)& aPipeline = myPipelineMap.FindKey (anIndex);
  if (myActivePipeline == aPipeline)
  {
    return myActivePipeline;
  }

  myActivePipeline = aPipeline;
  struct UniformsMatrixes
  {
    Graphic3d_Mat4 occWorldViewMatrix;
    Graphic3d_Mat4 occProjectionMatrix;
    Graphic3d_Mat4 occModelWorldMatrix;
  } aUniformsMatrixes;
  aUniformsMatrixes.occProjectionMatrix = myCamera->ProjectionMatrixF();
  aUniformsMatrixes.occWorldViewMatrix  = myCamera->OrientationMatrixF();
  aUboMatrixes->Init (myDevice, &aUniformsMatrixes, sizeof(aUniformsMatrixes));

  std::vector<VkDescriptorSet> aVkDescriptorSets (2, NULL);
  {
    std::vector<VkDescriptorSetLayout> aDescSetLayouts (aVkDescriptorSets.size(), aDescSetLayout->DescriptorSetLayout());
    VkDescriptorSetAllocateInfo aDescSetAllocInfo = {};
    aDescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    aDescSetAllocInfo.descriptorPool = myDescPool->DescriptorPool();
    aDescSetAllocInfo.descriptorSetCount = (uint32_t )aDescSetLayouts.size();
    aDescSetAllocInfo.pSetLayouts = aDescSetLayouts.data();

    VkResult aRes = vkAllocateDescriptorSets (myDevice->Device(), &aDescSetAllocInfo, aVkDescriptorSets.data());
    if (aRes != VK_SUCCESS)
    {
      logFailure ("failed to allocate descriptor sets", aRes);
      static const Handle(Vulkan_Pipeline) aDummy;
      return aDummy;
    }

    VkDescriptorBufferInfo aDescBuffInfos[2] = {};
    VkWriteDescriptorSet   aWriteDescSets[2] = {};
    {
      {
        VkDescriptorBufferInfo& aDescBuffInfo = aDescBuffInfos[0];
        aDescBuffInfo.buffer = aUboMatrixes->Buffer();
        aDescBuffInfo.offset = 0;
        aDescBuffInfo.range  = VK_WHOLE_SIZE;

        VkWriteDescriptorSet& aWriteDescSet = aWriteDescSets[0];
        aWriteDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        aWriteDescSet.dstSet = aVkDescriptorSets[0];
        aWriteDescSet.dstBinding = 0;
        aWriteDescSet.dstArrayElement = 0;
        aWriteDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        aWriteDescSet.descriptorCount = 1;
        aWriteDescSet.pBufferInfo = &aDescBuffInfo;
        aWriteDescSet.pImageInfo = NULL;
        aWriteDescSet.pTexelBufferView = NULL;
      }
      {
        VkDescriptorBufferInfo& aDescBuffInfo = aDescBuffInfos[1];
        aDescBuffInfo.buffer = myMaterials->Buffer();
        aDescBuffInfo.offset = 0;
        aDescBuffInfo.range  = myMaterials->Stride();

        VkWriteDescriptorSet& aWriteDescSet = aWriteDescSets[1];
        aWriteDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        aWriteDescSet.dstSet = aVkDescriptorSets[1];
        aWriteDescSet.dstBinding = 0;
        aWriteDescSet.dstArrayElement = 0;
        aWriteDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        aWriteDescSet.descriptorCount = 1;
        aWriteDescSet.pBufferInfo = &aDescBuffInfo;
        aWriteDescSet.pImageInfo = NULL;
        aWriteDescSet.pTexelBufferView = NULL;
      }
      vkUpdateDescriptorSets (myDevice->Device(), 2, aWriteDescSets, 0, NULL);
    }
  }

  uint32_t anOffsets[2] = { 0, myMaterials->Stride() * myActiveAspects->MaterialIndex() };
  myCmdBuffer->BindPipeline (aPipeline);
  /*vkCmdBindDescriptorSets (myCmdBuffer->CommandBuffer(),
                           VK_PIPELINE_BIND_POINT_GRAPHICS,
                           aPipeline->PipelineLayout()->PipelineLayout(),
                           0, 1, &aVkDescriptorSets[0], 2, anOffsets);*/
  vkCmdBindDescriptorSets (myCmdBuffer->CommandBuffer(),
                           VK_PIPELINE_BIND_POINT_GRAPHICS,
                           aPipeline->PipelineLayout()->PipelineLayout(),
                           0, 1, &aVkDescriptorSets[0], 1, &anOffsets[0]);
  vkCmdBindDescriptorSets (myCmdBuffer->CommandBuffer(),
                           VK_PIPELINE_BIND_POINT_GRAPHICS,
                           aPipeline->PipelineLayout()->PipelineLayout(),
                           1, 1, &aVkDescriptorSets[1], 1, &anOffsets[1]);
  return aPipeline;
}
