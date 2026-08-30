// Created by: Eugeny MALTCHIKOV
// Copyright (c) 2017 OPEN CASCADE SAS
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

#include <BOPAlgo_Options.hxx>
#include <Message_MsgFile.hxx>
#include <Message_ProgressScope.hxx>
#include <NCollection_BaseAllocator.hxx>
#include <TCollection_AsciiString.hxx>
#include <Precision.hxx>
#include <BOPAlgo_Alerts.hxx>

namespace
{
// Initialize textual messages for errors and warnings defined in BOPAlgo
#include "BOPAlgo_BOPAlgo_msg.pxx"

void BOPAlgo_LoadMessages()
{
  static const bool isLoaded = []() {
    if (!Message_MsgFile::HasMsg("BOPAlgo_LOAD_CHECKER"))
    {
      Message_MsgFile::LoadFromString(BOPAlgo_BOPAlgo_msg);
    }
    return true;
  }();
  (void)isLoaded;
}
} // namespace

//=================================================================================================

BOPAlgo_Options::BOPAlgo_Options()
    : myAllocator(NCollection_BaseAllocator::CommonBaseAllocator()),
      myReport(new Message_Report),
      myRunParallel(false),
      myFuzzyValue(Precision::Confusion()),
      myUseOBB(false)
{
  BOPAlgo_LoadMessages();
}

//=================================================================================================

BOPAlgo_Options::BOPAlgo_Options(const occ::handle<NCollection_BaseAllocator>& theAllocator)
    : myAllocator(theAllocator),
      myReport(new Message_Report),
      myRunParallel(false),
      myFuzzyValue(Precision::Confusion()),
      myUseOBB(false)
{
  BOPAlgo_LoadMessages();
}

//=================================================================================================

BOPAlgo_Options::~BOPAlgo_Options() = default;

//=================================================================================================

void BOPAlgo_Options::DumpErrors(Standard_OStream& theOS) const
{
  myReport->Dump(theOS, Message_Fail);
}

//=================================================================================================

void BOPAlgo_Options::DumpWarnings(Standard_OStream& theOS) const
{
  myReport->Dump(theOS, Message_Warning);
}

//=================================================================================================

void BOPAlgo_Options::SetFuzzyValue(const double theFuzz)
{
  myFuzzyValue = std::max(theFuzz, Precision::Confusion());
}

bool BOPAlgo_Options::UserBreak(const Message_ProgressScope& thePS)
{
  if (thePS.UserBreak())
  {
    AddError(new BOPAlgo_AlertUserBreak);
    return true;
  }
  return false;
}
