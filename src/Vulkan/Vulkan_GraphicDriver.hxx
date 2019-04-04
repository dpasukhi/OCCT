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

#ifndef _Vulkan_GraphicDriver_HeaderFile
#define _Vulkan_GraphicDriver_HeaderFile

#include <Graphic3d_GraphicDriver.hxx>

class Vulkan_Caps;
class Vulkan_Context;
class Vulkan_Device;
class Vulkan_Structure;
class Vulkan_View;
class Vulkan_Window;

//! This class defines an Vulkan graphic driver
class Vulkan_GraphicDriver : public Graphic3d_GraphicDriver
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_GraphicDriver, Graphic3d_GraphicDriver)
public:

  //! This is VK_MAKE_VERSION() redefinition.
  //! Note that VK_MAKE_VERSION() uses different bits comparing to OCC_VERSION_HEX.
  static uint32_t DefineVersion (uint32_t theMajor, uint32_t theMinor, uint32_t thePatch)
  {
    return ((theMajor << 22) | (theMinor << 12) | thePatch);
  }

public:

  //! Constructor.
  //! @param theAppName application name to be passed to Vulkan
  //! @param theAppVersion application version to be passed to Vulkan, see DefineVersion() method
  //! @param theDisp connection to display, required on Linux but optional on other systems
  Standard_EXPORT Vulkan_GraphicDriver (const TCollection_AsciiString& theAppName,
                                        const uint32_t theAppVersion,
                                        const Handle(Aspect_DisplayConnection)& theDisp);

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_GraphicDriver();

  //! Release default context.
  Standard_EXPORT void ReleaseContext();

  //! @return the visualization options
  Standard_EXPORT const Handle(Vulkan_Caps)& Options() const { return myCaps; }

  //! Perform initialization of default OpenGL context.
  Standard_EXPORT Standard_Boolean InitContext();

  //! Request limit of graphic resource of specific type.
  Standard_EXPORT virtual Standard_Integer InquireLimit (const Graphic3d_TypeOfLimit theType) const Standard_OVERRIDE;

public:

  Standard_EXPORT virtual Handle(Graphic3d_CStructure) CreateStructure (const Handle(Graphic3d_StructureManager)& theManager) Standard_OVERRIDE;

  Standard_EXPORT virtual void RemoveStructure (Handle(Graphic3d_CStructure)& theCStructure) Standard_OVERRIDE;

  Standard_EXPORT virtual Handle(Graphic3d_CView) CreateView (const Handle(Graphic3d_StructureManager)& theMgr) Standard_OVERRIDE;

  Standard_EXPORT virtual void RemoveView (const Handle(Graphic3d_CView)& theView) Standard_OVERRIDE;

public:

  Standard_EXPORT virtual void TextSize (const Handle(Graphic3d_CView)& theView,
                                         const Standard_CString         theText,
                                         const Standard_ShortReal       theHeight,
                                         Standard_ShortReal&            theWidth,
                                         Standard_ShortReal&            theAscent,
                                         Standard_ShortReal&            theDescent) const Standard_OVERRIDE;

  Standard_EXPORT virtual Standard_ShortReal DefaultTextHeight() const Standard_OVERRIDE { return 16.; }

  Standard_EXPORT virtual Standard_Boolean ViewExists (const Handle(Aspect_Window)& theWindow,
                                                       Handle(Graphic3d_CView)& theView) Standard_OVERRIDE;

public:

  //! Adds a new top-level z layer with ID theLayerId for all views. Z layers allow drawing structures in higher layers
  //! in foreground of structures in lower layers. To add a structure to desired layer on display it is necessary to
  //! set the layer index for the structure. The passed theLayerId should be not less than 0 (reserved for default layers).
  Standard_EXPORT virtual void AddZLayer (const Graphic3d_ZLayerId theLayerId) Standard_OVERRIDE;

  //! Removes Z layer. All structures displayed at the moment in layer will be displayed in
  //! default layer (the bottom-level z layer). By default, there are always default
  //! bottom-level layer that can't be removed.  The passed theLayerId should be not less than 0
  //! (reserved for default layers that can not be removed).
  Standard_EXPORT virtual void RemoveZLayer (const Graphic3d_ZLayerId theLayerId) Standard_OVERRIDE;

  //! Sets the settings for a single Z layer.
  Standard_EXPORT virtual void SetZLayerSettings (const Graphic3d_ZLayerId theLayerId, const Graphic3d_ZLayerSettings& theSettings) Standard_OVERRIDE;

public:

  //! Obsolete method.
  virtual void EnableVBO (const Standard_Boolean ) Standard_OVERRIDE {}

  //! Returns information about GPU memory usage.
  //! Please read OpenGl_Context::MemoryInfo() for more description.
  Standard_EXPORT virtual Standard_Boolean MemoryInfo (Standard_Size&           theFreeBytes,
                                                       TCollection_AsciiString& theInfo) const Standard_OVERRIDE;

public:

  const Handle(Vulkan_Device)& Device() const { return myVkDevice; }

  const Handle(Vulkan_Context)& SharedContext() const { return myContext; }

protected:

  Handle(Vulkan_Device)  myVkDevice;
  Handle(Vulkan_Context) myContext;

  Handle(Vulkan_Caps)                                      myCaps;
  NCollection_Map<Handle(Vulkan_View)>                     myMapOfView;
  NCollection_DataMap<Standard_Integer, Vulkan_Structure*> myMapOfStructure;

};

#endif // _Vulkan_GraphicDriver_HeaderFile
