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

#ifndef _Vulkan_MaterialUniformBuffer_HeaderFile
#define _Vulkan_MaterialUniformBuffer_HeaderFile

#include <Aspect_GenId.hxx>
#include <NCollection_Buffer.hxx>
#include <NCollection_Map.hxx>
#include <Vulkan_UniformBuffer.hxx>
#include <Quantity_ColorRGBA.hxx>
#include <Quantity_ColorRGBAHasher.hxx>

struct Vulkan_MaterialKey
{
  Quantity_ColorRGBA Color;
  Standard_Integer   Index;
  mutable Standard_Integer NbUsers;

  Vulkan_MaterialKey() : Index (0), NbUsers (0) {}

  //! Matching instances.
  bool IsEqual (const Vulkan_MaterialKey& theOther) const
  {
    return Color.IsEqual (theOther.Color);
  }

  //! Hash value, for Map interface.
  static Standard_Integer HashCode (const Quantity_ColorRGBA& theKey,
                                    const Standard_Integer theUpper)
  {
    uint32_t aHashCode = 0;
    aHashCode = aHashCode ^ Quantity_ColorRGBAHasher::HashCode (theKey, theUpper);
    return ::HashCode (Standard_Integer(aHashCode), theUpper);
  }

  //! Hash value, for Map interface.
  static Standard_Integer HashCode (const Vulkan_MaterialKey& theKey,
                                    const Standard_Integer theUpper)
  {
    return Vulkan_MaterialKey::HashCode (theKey.Color, theUpper);
  }

  //! Matching two instances, for Map interface.
  static bool IsEqual (const Vulkan_MaterialKey& theKey1,
                       const Vulkan_MaterialKey& theKey2)
  {
    return theKey1.IsEqual (theKey2);
  }
};

//! This class defines an Vulkan buffer.
class Vulkan_MaterialUniformBuffer : public Vulkan_UniformBuffer, protected NCollection_Map<Vulkan_MaterialKey, Vulkan_MaterialKey>
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_MaterialUniformBuffer, Vulkan_UniformBuffer)
public:
  DEFINE_STANDARD_ALLOC
  DEFINE_NCOLLECTION_ALLOC
public:

  //! Constructor.
  Standard_EXPORT Vulkan_MaterialUniformBuffer();

  Standard_EXPORT virtual ~Vulkan_MaterialUniformBuffer();

  Standard_EXPORT void SetAlignment (const Handle(Vulkan_Device)& theDevice);

  Standard_EXPORT bool Init (const Handle(Vulkan_Device)& theDevice);

  //! Return stride.
  uint32_t Stride() const { return myStride; }

  Standard_EXPORT Standard_Integer AddMaterial (const Quantity_ColorRGBA& theKey, Standard_Integer theOldIndex);

  Standard_EXPORT void ReleaseMaterial (Standard_Integer theOldIndex);

protected:

  using Vulkan_UniformBuffer::Init;

  Quantity_ColorRGBA* changeData (Standard_Integer theIndex) { return (Quantity_ColorRGBA* )(myBuffer.ChangeData() + size_t(myStride) * size_t(theIndex)); }

protected:

  NCollection_Buffer myBuffer;
  Aspect_GenId       myIdGenerator;
  Standard_Integer   myMinAlignment;
  Standard_Integer   myStride;
  Standard_Integer   myExtent;

};

#endif // _Vulkan_MaterialUniformBuffer_HeaderFile
