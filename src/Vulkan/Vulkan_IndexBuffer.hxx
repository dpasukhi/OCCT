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

#ifndef _Vulkan_IndexBuffer_HeaderFile
#define _Vulkan_IndexBuffer_HeaderFile

#include <Vulkan_Buffer.hxx>

class Graphic3d_IndexBuffer;

//! This class defines an Vulkan buffer.
class Vulkan_IndexBuffer : public Vulkan_Buffer
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_IndexBuffer, Vulkan_Buffer)
public:

  //! Constructor.
  Standard_EXPORT Vulkan_IndexBuffer();

  //! Init the object.
  Standard_EXPORT bool Init (const Handle(Vulkan_Device)& theDevice,
                             const Handle(Graphic3d_IndexBuffer)& theIndices);

  //! Return the index size.
  Standard_Integer Stride() const { return myStride; }

  //! Return number of indexes.
  Standard_Integer NbElements() const { return myNbElements; }

protected:

  Standard_Integer myStride;     //!< the index size
  Standard_Integer myNbElements; //!< number of indexes

};

#endif // _Vulkan_IndexBuffer_HeaderFile
