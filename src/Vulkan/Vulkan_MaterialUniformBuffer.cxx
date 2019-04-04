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

#include <Vulkan_MaterialUniformBuffer.hxx>

#include <NCollection_AlignedAllocator.hxx>
#include <TCollection.hxx>
#include <Vulkan_Device.hxx>

#include <vulkan/vulkan.h>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_MaterialUniformBuffer, Vulkan_UniformBuffer)

// =======================================================================
// function : Vulkan_MaterialUniformBuffer
// purpose  :
// =======================================================================
Vulkan_MaterialUniformBuffer::Vulkan_MaterialUniformBuffer()
: myBuffer (NCollection_BaseAllocator::CommonBaseAllocator()),
  myMinAlignment (0),
  myStride (sizeof(Graphic3d_Vec4)),
  myExtent (0)
{
  //
}

// =======================================================================
// function : ~Vulkan_MaterialUniformBuffer
// purpose  :
// =======================================================================
Vulkan_MaterialUniformBuffer::~Vulkan_MaterialUniformBuffer()
{
  //
}

// =======================================================================
// function : SetAlignment
// purpose  :
// =======================================================================
void Vulkan_MaterialUniformBuffer::SetAlignment (const Handle(Vulkan_Device)& theDevice)
{
  const Standard_Integer anAlign = (Standard_Integer )theDevice->MinUniformBufferOffsetAlignment();
  Handle(NCollection_AlignedAllocator) anAlloc = new NCollection_AlignedAllocator (anAlign);
  myBuffer.SetAllocator (anAlloc);
  myStride = Max ((Standard_Integer )sizeof(Graphic3d_Vec4), anAlign);
  myExtent = 0;
  AddMaterial (Quantity_ColorRGBA (1.0f, 1.0f, 1.0f, 1.0f), -1);
}

// =======================================================================
// function : Init
// purpose  :
// =======================================================================
bool Vulkan_MaterialUniformBuffer::Init (const Handle(Vulkan_Device)& theDevice)
{
  return Init (theDevice, myBuffer.Data(), size_t(myStride) * size_t(myExtent), myBuffer.Size());
}

// =======================================================================
// function : ReleaseMaterial
// purpose  :
// =======================================================================
void Vulkan_MaterialUniformBuffer::ReleaseMaterial (Standard_Integer theOldIndex)
{
  if (theOldIndex == -1)
  {
    return;
  }

  const Quantity_ColorRGBA* aColor = changeData (theOldIndex);
  for (MapNode* aNodeIter = !IsEmpty() ? (MapNode* )myData1[Vulkan_MaterialKey::HashCode (*aColor, NbBuckets())] : NULL;
       aNodeIter != NULL; aNodeIter = (MapNode* )aNodeIter->Next())
  {
    const Vulkan_MaterialKey& aKey = aNodeIter->Key();
    if (aKey.Index == theOldIndex)
    {
      if (--aKey.NbUsers == 0)
      {
        myIdGenerator.Free (theOldIndex);
        --myExtent;

        Vulkan_MaterialKey aKeyRemove;
        aKeyRemove.Color = *aColor;
        Remove (aKeyRemove);
      }
      return;
    }
  }
}

// =======================================================================
// function : AddMaterial
// purpose  :
// =======================================================================
Standard_Integer Vulkan_MaterialUniformBuffer::AddMaterial (const Quantity_ColorRGBA& theKey, Standard_Integer theOldIndex)
{
  if (theOldIndex != -1)
  {
    if (changeData (theOldIndex)->IsEqual (theKey))
    {
      return theOldIndex;
    }
    ReleaseMaterial (theOldIndex);
  }

  for (MapNode* aNodeIter = !IsEmpty() ? (MapNode* )myData1[Vulkan_MaterialKey::HashCode (theKey, NbBuckets())] : NULL;
       aNodeIter != NULL; aNodeIter = (MapNode* )aNodeIter->Next())
  {
    const Vulkan_MaterialKey& aKey = aNodeIter->Key();
    if (aKey.Color.IsEqual (theKey))
    {
      ++aKey.NbUsers;
      return aKey.Index;
    }
  }

  size_t aNewSize = size_t(myExtent + 1) * size_t(myStride);
  if (aNewSize > myBuffer.Size())
  {
    const Standard_Integer anExtent = TCollection::NextPrimeForMap (myExtent);
    aNewSize = size_t(anExtent) * size_t(myStride);

    myBuffer.Allocate (aNewSize);
    for (NCollection_Map<Vulkan_MaterialKey, Vulkan_MaterialKey>::Iterator aMapIter (*this); aMapIter.More(); aMapIter.Next())
    {
      Quantity_ColorRGBA* aColor = changeData (aMapIter.Key().Index);
      *aColor = aMapIter.Key().Color;
    }
  }

  ++myExtent;
  Vulkan_MaterialKey aKey;
  aKey.Color = theKey;
  aKey.NbUsers = 1;
  aKey.Index = myIdGenerator.Next();
  Add (aKey);

  *changeData (aKey.Index) = theKey;
  return aKey.Index;
}
