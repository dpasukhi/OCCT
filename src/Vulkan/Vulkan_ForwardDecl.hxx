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

#ifndef _Vulkan_ForwardDecl_HeaderFile
#define _Vulkan_ForwardDecl_HeaderFile

#include <Standard_TypeDef.hxx>

#ifndef VK_DEFINE_HANDLE
  #define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;

  #if !defined(VK_DEFINE_NON_DISPATCHABLE_HANDLE)
  #if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__) ) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
    #define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T *object;
  #else
    #define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;
  #endif
  #endif

  VK_DEFINE_HANDLE(VkInstance)
  VK_DEFINE_HANDLE(VkPhysicalDevice)
  VK_DEFINE_HANDLE(VkDevice)
  VK_DEFINE_HANDLE(VkQueue)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSemaphore)
  VK_DEFINE_HANDLE(VkCommandBuffer)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkFence)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDeviceMemory)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkBuffer)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkImage)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkEvent)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkQueryPool)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkBufferView)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkImageView)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkShaderModule)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPipelineCache)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPipelineLayout)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkRenderPass)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPipeline)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDescriptorSetLayout)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSampler)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDescriptorPool)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDescriptorSet)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkFramebuffer)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkCommandPool)

  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSurfaceKHR)
  VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSwapchainKHR)

  struct VkAllocationCallbacks;
  struct VkMemoryRequirements;
  struct VkPhysicalDeviceMemoryProperties;
  struct VkSurfaceFormatKHR;

  typedef uint64_t VkDeviceSize;
#endif

#endif // _Vulkan_ForwardDecl_HeaderFile
