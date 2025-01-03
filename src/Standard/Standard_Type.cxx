// Copyright (c) 1998-1999 Matra Datavision
// Copyright (c) 1999-2025 OPEN CASCADE SAS
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

#include <Standard_Assert.hxx>
#include <Standard_Mutex.hxx>
#include <Standard_Type.hxx>

#include <unordered_map>

IMPLEMENT_STANDARD_RTTIEXT(Standard_Type, Standard_Transient)

Standard_Type::Standard_Type(const std::type_info&        theInfo,
                             const char*                  theName,
                             Standard_Size                theSize,
                             const Handle(Standard_Type)& theParent)
: myInfo(theInfo),
  myName(theName),
  mySize(theSize),
  myLevel(theParent.IsNull() ? 0 : theParent->myLevel + 1),
  myParent(theParent)
{}

Standard_Boolean Standard_Type::SubType(const Handle(Standard_Type)& theOther) const
{
  const Standard_Type* aTypeIter = this;
  while (aTypeIter && theOther->myLevel <= aTypeIter->myLevel && theOther->mySize <= aTypeIter->mySize)
  {
    if (theOther.get() == aTypeIter)
    {
      return true;
    }
    aTypeIter = aTypeIter->Parent().get();
  }
  return false;
}

Standard_Boolean Standard_Type::SubType(const Standard_CString theName) const
{
  const Standard_Type* aTypeIter = this;
  do
  {
    if (IsEqual(theName, aTypeIter->Name()))
    {
      return true;
    }
    aTypeIter = aTypeIter->Parent().get();
  } while (aTypeIter);
  return false;
}

void Standard_Type::Print(Standard_OStream& AStream) const
{
  AStream << std::hex << (Standard_Address)this << " : " << std::dec << myName;
}

namespace
{
  // Map of string to type
  typedef std::unordered_map<std::type_index, Standard_Type*> registry_type;

  // Registry is made static in the function to ensure that it gets
  // initialized by the time of first access
  registry_type& GetRegistry()
  {
    static registry_type theRegistry;
    return theRegistry;
  }

  Standard_Mutex& GetRegistrationMutex()
  {
    static Standard_Mutex theMutex;
    return theMutex;
  }

  // To initialize theRegistry map as soon as possible to be destroyed the latest
  Handle(Standard_Type) theType = STANDARD_TYPE(Standard_Transient);
}

Standard_Type* Standard_Type::Register(const std::type_info&        theInfo,
                                       const char*                  theName,
                                       Standard_Size                theSize,
                                       const Handle(Standard_Type)& theParent)
{
  // Access to registry is protected by mutex; it should not happen often because
  // instances are cached by Standard_Type::Instance() (one per binary module)

  // return existing descriptor if already in the registry
  registry_type& aRegistry = GetRegistry();
  Standard_Type* aType     = 0;

  Standard_Mutex::Sentry aSentry(GetRegistrationMutex());
  auto                   anIter = aRegistry.find(theInfo);
  if (anIter != aRegistry.end())
  {
    return anIter->second;
  }

  // else create a new descriptor
  aType = new Standard_Type(theInfo, theName, theSize, theParent);

  // then add it to registry and return (the reference to the handle stored in the registry)
  aRegistry.emplace(theInfo, aType);
  return aType;
}

Standard_Type::~Standard_Type()
{
  // remove descriptor from the registry
  registry_type&         aRegistry = GetRegistry();
  Standard_Mutex::Sentry aSentry(GetRegistrationMutex());
  Standard_ASSERT(aRegistry.erase(myInfo) > 0, "Standard_Type::~Standard_Type() cannot find itself in registry", );
}
