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

#ifndef _Vulkan_PrimitiveArray_HeaderFile
#define _Vulkan_PrimitiveArray_HeaderFile

#include <Graphic3d_TypeOfPrimitiveArray.hxx>
#include <Vulkan_Element.hxx>

class Graphic3d_Buffer;
class Graphic3d_BoundBuffer;
class Graphic3d_IndexBuffer;
class Vulkan_IndexBuffer;
class Vulkan_VertexBuffer;

//! Drawable primitive array.
class Vulkan_PrimitiveArray : public Vulkan_Element
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_PrimitiveArray, Vulkan_Element)
public:

  //! Main constructor.
  Standard_EXPORT Vulkan_PrimitiveArray (const Graphic3d_TypeOfPrimitiveArray theType,
                                         const Handle(Graphic3d_IndexBuffer)& theIndices,
                                         const Handle(Graphic3d_Buffer)&      theAttribs,
                                         const Handle(Graphic3d_BoundBuffer)& theBounds);

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_PrimitiveArray();

  //! Release GPU resources.
  Standard_EXPORT virtual void Release() Standard_OVERRIDE;

  //! Perform rendering.
  Standard_EXPORT virtual void Render (const Handle(Vulkan_Context)& theCtx) Standard_OVERRIDE;

  //! Return vertex buffer object
  const Handle(Vulkan_VertexBuffer)& AttributesVbo() const { return myVboAttribs; }

  //! Return index buffer object
  const Handle(Vulkan_IndexBuffer)& IndexVbo() const { return myVboIndices; }

protected:

  Handle(Graphic3d_IndexBuffer) myIndices;
  Handle(Graphic3d_Buffer)      myAttribs;
  Handle(Graphic3d_BoundBuffer) myBounds;

  Handle(Vulkan_VertexBuffer) myVboAttribs;
  Handle(Vulkan_IndexBuffer)  myVboIndices;
  Graphic3d_TypeOfPrimitiveArray myType;

};

#endif // _Vulkan_PrimitiveArray_HeaderFile
