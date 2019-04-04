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

#ifndef _Vulkan_PipelineLayout_HeaderFile
#define _Vulkan_PipelineLayout_HeaderFile

#include <Vulkan_Object.hxx>

class Vulkan_DescriptorSetLayout;

//! This class defines an Vulkan Pipeline Layout.
class Vulkan_PipelineLayout : public Vulkan_Object
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_PipelineLayout, Vulkan_Object)
public:

  //! Constructor.
  Standard_EXPORT Vulkan_PipelineLayout();

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_PipelineLayout();

  //! Return object.
  VkPipelineLayout PipelineLayout() const { return myVkPipelineLayout; }

  //! Create the object, @sa vkCreatePipelineLayout().
  Standard_EXPORT bool Create (const Handle(Vulkan_Device)& theDevice,
                               const Handle(Vulkan_DescriptorSetLayout)& theDescSetLayout);

protected:

  //! Release the object.
  virtual void release() Standard_OVERRIDE { releasePipelineLayout(); }

  //! Release the object, @sa vkDestroyPipelineLayout().
  Standard_EXPORT void releasePipelineLayout();

protected:

  Handle(Vulkan_DescriptorSetLayout) myDescSetLayout;
  VkPipelineLayout myVkPipelineLayout;

};

#endif // _Vulkan_PipelineLayout_HeaderFile
