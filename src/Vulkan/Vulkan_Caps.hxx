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

#ifndef _Vulkan_Caps_HeaderFile
#define _Vulkan_Caps_HeaderFile

#include <Standard_Type.hxx>
#include <Standard_Transient.hxx>
#include <TCollection_AsciiString.hxx>

//! Class to define graphic driver capabilities.
//! Notice that these options will be ignored if particular functionality does not provided by Vulkan driver
class Vulkan_Caps : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_Caps, Standard_Transient)
public: //! @name flags to disable particular functionality, should be used only for testing purposes!

  Standard_Integer swapInterval;      //!< controls swap interval - 0 for VSync off and 1 for VSync on, 1 by default

public: //! @name context creation parameters

  //! Specify that driver should not swap back/front buffers at the end of frame.
  //! Useful when OCCT Viewer is integrated into existing Vulkan rendering pipeline as part,
  //! thus swapping part is performed outside.
  //!
  //! OFF by default.
  Standard_Boolean buffersNoSwap;

  //! Request debug Vulkan context. This flag requires support of necessary layers in Vulkan driver.
  //!
  //! When turned on Vulkan driver emits error and warning messages to provided callback.
  //! Affects performance - thus should not be turned on by products in released state.
  //!
  //! OFF by default.
  Standard_Boolean contextDebug;

  //! Disable hardware acceleration.
  //!
  //! Flags to intentionally look for VK_PHYSICAL_DEVICE_TYPE_CPU devices.
  //!
  //! OFF by default.
  Standard_Boolean contextNoAccel;

  //! Look for device with specified name.
  //!
  //! EMPTY by default.
  TCollection_AsciiString contextDevice;

public: //! @name flags to activate verbose output

  //! Print GLSL program compilation/linkage warnings, if any. OFF by default.
  Standard_Boolean glslWarnings;

  //! Suppress redundant messages from debug Vulkan context. ON by default.
  Standard_Boolean suppressExtraMsg;

public: //! @name class methods

  //! Default constructor - initialize with most optimal values.
  Standard_EXPORT Vulkan_Caps();

  //! Destructor.
  Standard_EXPORT virtual ~Vulkan_Caps();

  //! Copy maker.
  Standard_EXPORT Vulkan_Caps& operator= (const Vulkan_Caps& theCopy);

private:

  //! Not implemented
  Vulkan_Caps (const Vulkan_Caps& );

};

#endif // _Vulkan_Caps_HeaderFile
