// Copyright (c) 2024 OPEN CASCADE SAS
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

#ifndef NCollection_SeqNode_HeaderFile
#define NCollection_SeqNode_HeaderFile

#include <NCollection_BaseAllocator.hxx>
#include <NCollection_DefineAlloc.hxx>

//! Class used to  represent a node  in the BaseSeq and extend
//! some map nodes.
class NCollection_SeqNode
{
public:
  // define new operator for use with NCollection allocators
  DEFINE_NCOLLECTION_ALLOC
public:
  NCollection_SeqNode (NCollection_SeqNode* theNext, NCollection_SeqNode* thePrev) :
    myNext (theNext), myPrevious (thePrev) {}
  NCollection_SeqNode () : myNext (nullptr), myPrevious (nullptr) {}
  NCollection_SeqNode * Next      () const { return myNext; }
  NCollection_SeqNode * Previous  () const { return myPrevious; }
  void SetNext     (NCollection_SeqNode * theNext) { myNext = theNext; }
  void SetPrevious (NCollection_SeqNode * thePrev) { myPrevious = thePrev; }
  
 private:
  NCollection_SeqNode* myNext;
  NCollection_SeqNode* myPrevious;
};

#endif
