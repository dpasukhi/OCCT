// Created on: 2000-04-18
// Created by: Andrey BETENEV
// Copyright (c) 2000-2014 OPEN CASCADE SAS
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

#ifndef _StepShape_DimensionalSizeWithPath_HeaderFile
#define _StepShape_DimensionalSizeWithPath_HeaderFile

#include <Standard.hxx>
#include <Standard_Type.hxx>

#include <StepShape_DimensionalSize.hxx>
class StepRepr_ShapeAspect;
class TCollection_HAsciiString;

class StepShape_DimensionalSizeWithPath;
DEFINE_STANDARD_HANDLE(StepShape_DimensionalSizeWithPath, StepShape_DimensionalSize)

//! Representation of STEP entity DimensionalSizeWithPath
class StepShape_DimensionalSizeWithPath : public StepShape_DimensionalSize
{

public:
  //! Empty constructor
  Standard_EXPORT StepShape_DimensionalSizeWithPath();

  //! Initialize all fields (own and inherited)
  Standard_EXPORT void Init(const Handle(StepRepr_ShapeAspect)&     aDimensionalSize_AppliesTo,
                            const Handle(TCollection_HAsciiString)& aDimensionalSize_Name,
                            const Handle(StepRepr_ShapeAspect)&     aPath);

  //! Returns field Path
  Standard_EXPORT Handle(StepRepr_ShapeAspect) Path() const;

  //! Set field Path
  Standard_EXPORT void SetPath(const Handle(StepRepr_ShapeAspect)& Path);

  DEFINE_STANDARD_RTTIEXT(StepShape_DimensionalSizeWithPath, StepShape_DimensionalSize)

protected:
private:
  Handle(StepRepr_ShapeAspect) thePath;
};

#endif // _StepShape_DimensionalSizeWithPath_HeaderFile
