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

#ifndef _Vulkan_DescriptorSetLayout_HeaderFile
#define _Vulkan_DescriptorSetLayout_HeaderFile

#include <Vulkan_Object.hxx>

//! This class defines an Vulkan descriptor set layout.
class Vulkan_DescriptorSetLayout : public Vulkan_Object
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_DescriptorSetLayout, Vulkan_Object)
public:

  //! Constructor.
  Standard_EXPORT Vulkan_DescriptorSetLayout();

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_DescriptorSetLayout();

  //! Return object.
  const VkDescriptorSetLayout& DescriptorSetLayout() const { return myVkDescSetLayout; }

  //! Create the object, @sa vkCreateDescriptorSetLayout().
  Standard_EXPORT bool Create (const Handle(Vulkan_Device)& theDevice);

protected:

  //! Release the object.
  virtual void release() Standard_OVERRIDE { releaseLayout(); }

  //! Release the object, @sa vkDestroyDescriptorSetLayout().
  Standard_EXPORT void releaseLayout();

protected:

  VkDescriptorSetLayout myVkDescSetLayout;

};

#endif // _Vulkan_DescriptorSetLayout_HeaderFile
