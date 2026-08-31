// Copyright (c) 2026 OPEN CASCADE SAS
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

#include <BRepGraph_ItemStamp.hxx>

#include <Standard_HashUtils.hxx>

bool BRepGraph_ItemStamp::operator==(const BRepGraph_ItemStamp& theOther) const
{
  if (!IsValid() && !theOther.IsValid())
  {
    return true;
  }
  if (myDomain != theOther.myDomain)
  {
    return false;
  }
  if (myMutationGen != theOther.myMutationGen || myGeneration != theOther.myGeneration
      || myRuntimeIdentity != theOther.myRuntimeIdentity)
  {
    return false;
  }
  if (myDomain == Domain::Node)
  {
    return myNodeUID == theOther.myNodeUID;
  }
  if (myDomain == Domain::Reference)
  {
    return myRefUID == theOther.myRefUID;
  }
  return myNodeUID == theOther.myNodeUID && myRefUID == theOther.myRefUID;
}

//=================================================================================================

bool BRepGraph_ItemStamp::IsSameItem(const BRepGraph_ItemStamp& theOther) const
{
  if (myDomain != theOther.myDomain)
  {
    return false;
  }
  if (myDomain == Domain::Node)
  {
    return myNodeUID == theOther.myNodeUID;
  }
  if (myDomain == Domain::Reference)
  {
    return myRefUID == theOther.myRefUID;
  }
  return myNodeUID == theOther.myNodeUID && myRefUID == theOther.myRefUID;
}

//=================================================================================================

size_t BRepGraph_ItemStamp::HashValue() const
{
  size_t aCombination[5];
  aCombination[0] = opencascade::hash(static_cast<int>(myDomain));
  if (myDomain == Domain::Node)
  {
    aCombination[1] = myNodeUID.HashValue();
  }
  else if (myDomain == Domain::Reference)
  {
    aCombination[1] = myRefUID.HashValue();
  }
  else
  {
    aCombination[1] = opencascade::hash(0);
  }
  aCombination[2] = opencascade::hash(myMutationGen);
  aCombination[3] = opencascade::hash(myGeneration);
  aCombination[4] = std::hash<Standard_GUID>{}(myRuntimeIdentity);
  return opencascade::hashBytes(aCombination, sizeof(aCombination));
}
