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

#include <Vulkan_GraphicDriver.hxx>

#include <Graphic3d_StructureManager.hxx>
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Vulkan_Caps.hxx>
#include <Vulkan_Context.hxx>
#include <Vulkan_Device.hxx>
#include <Vulkan_View.hxx>
#include <Vulkan_Structure.hxx>
#include <Standard_Version.hxx>

#if defined(_WIN32) && !defined(OCCT_UWP)
  #include <WNT_Window.hxx>
#elif defined(__APPLE__) && !defined(MACOSX_USE_GLX)
  #include <Cocoa_Window.hxx>
#elif defined(__ANDROID__) || defined(__QNX__) || defined(OCCT_UWP)
  //
#else
  #include <Xw_Window.hxx>
#endif

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_GraphicDriver, Graphic3d_GraphicDriver)

// =======================================================================
// function : Vulkan_GraphicDriver
// purpose  :
// =======================================================================
Vulkan_GraphicDriver::Vulkan_GraphicDriver (const TCollection_AsciiString& theAppName,
                                            const uint32_t theAppVersion,
                                            const Handle(Aspect_DisplayConnection)& theDisp)
: Graphic3d_GraphicDriver (theDisp),
  myVkDevice (new Vulkan_Device (theAppName, theAppVersion, "Open CASCADE Technology",
                                 DefineVersion (OCC_VERSION_MAJOR, OCC_VERSION_MINOR, OCC_VERSION_MAINTENANCE))),
  myContext (new Vulkan_Context()),
  myCaps (new Vulkan_Caps()),
  myMapOfView      (1, NCollection_BaseAllocator::CommonBaseAllocator()),
  myMapOfStructure (1, NCollection_BaseAllocator::CommonBaseAllocator())
{
  //
}

// =======================================================================
// function : ~Vulkan_GraphicDriver
// purpose  :
// =======================================================================
Vulkan_GraphicDriver::~Vulkan_GraphicDriver()
{
  ReleaseContext();
}

// =======================================================================
// function : ReleaseContext
// purpose  :
// =======================================================================
void Vulkan_GraphicDriver::ReleaseContext()
{
  myVkDevice->Release();
}

// =======================================================================
// function : InitContext
// purpose  :
// =======================================================================
Standard_Boolean Vulkan_GraphicDriver::InitContext()
{
  ReleaseContext();
  if (myVkDevice->Init (myCaps))
  {
    myContext->Init (myVkDevice);
    return true;
  }
  return false;
}

// =======================================================================
// function : InquireLimit
// purpose  :
// =======================================================================
Standard_Integer Vulkan_GraphicDriver::InquireLimit (const Graphic3d_TypeOfLimit theType) const
{
  (void )theType;
  return 0;
}

// =======================================================================
// function : MemoryInfo
// purpose  :
// =======================================================================
Standard_Boolean Vulkan_GraphicDriver::MemoryInfo (Standard_Size& theFreeBytes,
                                                   TCollection_AsciiString& theInfo) const
{
  (void )theFreeBytes;
  (void )theInfo;
  return false;
}

// =======================================================================
// function : TextSize
// purpose  :
// =======================================================================
void Vulkan_GraphicDriver::TextSize (const Handle(Graphic3d_CView)& theView,
                                     const Standard_CString theText,
                                     const Standard_ShortReal theHeight,
                                     Standard_ShortReal& theWidth,
                                     Standard_ShortReal& theAscent,
                                     Standard_ShortReal& theDescent) const
{
  (void )theView;
  (void )theText;
  (void )theHeight;
  (void )theWidth;
  (void )theAscent;
  (void )theDescent;
}

//=======================================================================
//function : AddZLayer
//purpose  :
//=======================================================================
void Vulkan_GraphicDriver::AddZLayer (const Graphic3d_ZLayerId theLayerId)
{
  if (theLayerId < 1)
  {
    Standard_ASSERT_RAISE (theLayerId > 0, "Vulkan_GraphicDriver::AddZLayer, negative and zero IDs are reserved");
  }

  myLayerIds.Add (theLayerId);

  // Default z-layer settings
  myMapOfZLayerSettings.Bind (theLayerId, Graphic3d_ZLayerSettings());
  addZLayerIndex (theLayerId);

  // Add layer to all views
  for (NCollection_Map<Handle(Vulkan_View)>::Iterator aViewIt (myMapOfView); aViewIt.More(); aViewIt.Next())
  {
    aViewIt.Value()->AddZLayer (theLayerId);
  }
}

//=======================================================================
//function : RemoveZLayer
//purpose  :
//=======================================================================
void Vulkan_GraphicDriver::RemoveZLayer (const Graphic3d_ZLayerId theLayerId)
{
  Standard_ASSERT_RAISE (theLayerId > 0,
                         "Vulkan_GraphicDriver::AddZLayer, negative and zero IDs are reserved and can not be removed");

  Standard_ASSERT_RAISE (myLayerIds.Contains (theLayerId),
                         "Vulkan_GraphicDriver::RemoveZLayer, Layer with theLayerId does not exist");

  // Remove layer from all of the views
  for (NCollection_Map<Handle(Vulkan_View)>::Iterator aViewIt (myMapOfView); aViewIt.More(); aViewIt.Next())
  {
    aViewIt.Value()->RemoveZLayer (theLayerId);
  }

  // Unset Z layer for all of the structures.
  for (NCollection_DataMap<Standard_Integer, Vulkan_Structure*>::Iterator aStructIt (myMapOfStructure); aStructIt.More(); aStructIt.Next())
  {
    Vulkan_Structure* aStruct = aStructIt.ChangeValue();
    if (aStruct->ZLayer() == theLayerId)
    {
      aStruct->SetZLayer (Graphic3d_ZLayerId_Default);
    }
  }

  // Remove index
  for (TColStd_SequenceOfInteger::Iterator aLayerIt (myLayerSeq); aLayerIt.More(); aLayerIt.Next())
  {
    if (aLayerIt.Value() == theLayerId)
    {
      myLayerSeq.Remove (aLayerIt);
      break;
    }
  }

  myMapOfZLayerSettings.UnBind (theLayerId);
  myLayerIds.Remove  (theLayerId);
}

//=======================================================================
//function : SetZLayerSettings
//purpose  :
//=======================================================================
void Vulkan_GraphicDriver::SetZLayerSettings (const Graphic3d_ZLayerId theLayerId,
                                              const Graphic3d_ZLayerSettings& theSettings)
{
  base_type::SetZLayerSettings (theLayerId, theSettings);

  // Change Z layer settings in all managed views
  for (NCollection_Map<Handle(Vulkan_View)>::Iterator aViewIt (myMapOfView); aViewIt.More(); aViewIt.Next())
  {
    aViewIt.Value()->SetZLayerSettings (theLayerId, theSettings);
  }
}

// =======================================================================
// function : Structure
// purpose  :
// =======================================================================
Handle(Graphic3d_CStructure) Vulkan_GraphicDriver::CreateStructure (const Handle(Graphic3d_StructureManager)& theManager)
{
  Handle(Vulkan_Structure) aStructure = new Vulkan_Structure (theManager);
  myMapOfStructure.Bind (aStructure->Id, aStructure.get());
  return aStructure;
}

// =======================================================================
// function : Structure
// purpose  :
// =======================================================================
void Vulkan_GraphicDriver::RemoveStructure (Handle(Graphic3d_CStructure)& theCStructure)
{
  Vulkan_Structure* aStructure = NULL;
  if (!myMapOfStructure.Find (theCStructure->Id, aStructure))
  {
    return;
  }

  myMapOfStructure.UnBind (theCStructure->Id);
  aStructure->Release();
  theCStructure.Nullify();
}

// =======================================================================
// function : View
// purpose  :
// =======================================================================
Handle(Graphic3d_CView) Vulkan_GraphicDriver::CreateView (const Handle(Graphic3d_StructureManager)& theMgr)
{
  Handle(Vulkan_View) aView = new Vulkan_View (theMgr, this);

  myMapOfView.Add (aView);

  for (TColStd_SequenceOfInteger::Iterator aLayerIt (myLayerSeq); aLayerIt.More(); aLayerIt.Next())
  {
    const Graphic3d_ZLayerId        aLayerID  = aLayerIt.Value();
    const Graphic3d_ZLayerSettings& aSettings = myMapOfZLayerSettings.Find (aLayerID);
    aView->AddZLayer         (aLayerID);
    aView->SetZLayerSettings (aLayerID, aSettings);
  }

  return aView;
}

// =======================================================================
// function : RemoveView
// purpose  :
// =======================================================================
void Vulkan_GraphicDriver::RemoveView (const Handle(Graphic3d_CView)& theView)
{
  Handle(Vulkan_View) aView = Handle(Vulkan_View)::DownCast (theView);
  if (aView.IsNull())
  {
    return;
  }

  if (!myMapOfView.Remove (aView))
  {
    return;
  }

  aView->ReleaseVkResources();
  if (myMapOfView.IsEmpty())
  {
    // The last view removed but some objects still present.
    // Release GL resources now without object destruction.
    for (NCollection_DataMap<Standard_Integer, Vulkan_Structure*>::Iterator aStructIt (myMapOfStructure); aStructIt.More (); aStructIt.Next())
    {
      Vulkan_Structure* aStruct = aStructIt.ChangeValue();
      aStruct->ReleaseVkResources();
    }

    if (!myMapOfStructure.IsEmpty())
    {
      aView->StructureManager()->SetDeviceLost();
    }
  }
}

//=======================================================================
//function : ViewExists
//purpose  :
//=======================================================================
Standard_Boolean Vulkan_GraphicDriver::ViewExists (const Handle(Aspect_Window)& theWindow,
                                                   Handle(Graphic3d_CView)& theView)
{
  // Parse the list of views to find a view with the specified window
#if defined(_WIN32) && !defined(OCCT_UWP)
  const Handle(WNT_Window) aWindowToFind = Handle(WNT_Window)::DownCast (theWindow);
  Aspect_Handle aWindowIdToFind = aWindowToFind->HWindow();
#elif defined(__APPLE__) && !defined(MACOSX_USE_GLX)
  const Handle(Cocoa_Window) aWindowToFind = Handle(Cocoa_Window)::DownCast (theWindow);
  #if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
    UIView* aWindowIdToFind = aWindowToFind->HView();
  #else
    NSView* aWindowIdToFind = aWindowToFind->HView();
  #endif
#elif defined(__ANDROID__) || defined(__QNX__) || defined(OCCT_UWP)
  (void )theWindow;
  int aWindowIdToFind = -1;
#else
  const Handle(Xw_Window) aWindowToFind = Handle(Xw_Window)::DownCast (theWindow);
  int aWindowIdToFind = int (aWindowToFind->XWindow());
#endif
  for (NCollection_Map<Handle(Vulkan_View)>::Iterator aViewIt (myMapOfView); aViewIt.More(); aViewIt.Next())
  {
    const Handle(Vulkan_View)& aView = aViewIt.Value();
    if (aView->IsDefined()
     && aView->IsActive())
    {
      const Handle(Aspect_Window) anAspectWindow = aView->Window();

    #if defined(_WIN32) && !defined(OCCT_UWP)
      const Handle(WNT_Window) aWindow = Handle(WNT_Window)::DownCast (anAspectWindow);
      Aspect_Handle aWindowIdOfView = aWindow->HWindow();
    #elif defined(__APPLE__) && !defined(MACOSX_USE_GLX)
      const Handle(Cocoa_Window) aWindow = Handle(Cocoa_Window)::DownCast (anAspectWindow);
      #if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
        UIView* aWindowIdOfView = aWindow->HView();
      #else
        NSView* aWindowIdOfView = aWindow->HView();
      #endif
    #elif defined(__ANDROID__) || defined(__QNX__) || defined(OCCT_UWP)
      int aWindowIdOfView = 0;
    #else
      const Handle(Xw_Window) aWindow = Handle(Xw_Window)::DownCast (anAspectWindow);
      int aWindowIdOfView = int(aWindow->XWindow());
    #endif
      if (aWindowIdOfView == aWindowIdToFind)
      {
        theView = aView;
        return true;
      }
    }
  }
  return false;
}
