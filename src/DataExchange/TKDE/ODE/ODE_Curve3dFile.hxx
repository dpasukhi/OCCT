// Copyright (c) 2025 OPEN CASCADE SAS
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

#ifndef _ODE_Curve3dFile_HeaderFile
#define _ODE_Curve3dFile_HeaderFile

#include <Standard.hxx>
#include <Standard_Transient.hxx>
#include <TCollection_AsciiString.hxx>
#include <NCollection_Sequence.hxx>
#include <Geom_Curve.hxx>
#include <ODE_Status.hxx>
#include <ODE_ObjectRef.hxx>
#include <unordered_map>

//! Manages serialization and deserialization of Geom_Curve objects
//! to Cap'n Proto format with deduplication support.
class ODE_Curve3dFile : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(ODE_Curve3dFile, Standard_Transient)

public:
  //! Constructor
  Standard_EXPORT ODE_Curve3dFile();

  //! Adds a 3D curve to the file with deduplication.
  //! Returns an object reference that can be used to retrieve the curve later.
  //! @param theCurve Curve to add
  //! @return Object reference to the curve
  Standard_EXPORT ODE_ObjectRef AddCurve(const Handle(Geom_Curve)& theCurve);

  //! Gets a curve by its object reference.
  //! @param theRef Object reference
  //! @return Curve handle (may be null if reference is invalid)
  Standard_EXPORT Handle(Geom_Curve) GetCurve(const ODE_ObjectRef& theRef) const;

  //! Gets total number of unique curves
  Standard_EXPORT int CurveCount() const;

  //! Gets total number of curve instances (including duplicates)
  Standard_EXPORT int InstanceCount() const;

  //! Clears all curves and resets the file
  Standard_EXPORT void Clear();

  //! Writes curves to a Cap'n Proto binary file
  //! @param thePath Path to the output file
  //! @return Status code
  Standard_EXPORT ODE_Status WriteToFile(const TCollection_AsciiString& thePath) const;

  //! Reads curves from a Cap'n Proto binary file
  //! @param thePath Path to the input file
  //! @return Status code
  Standard_EXPORT ODE_Status ReadFromFile(const TCollection_AsciiString& thePath);

private:
  //! Internal storage for curve and its handle instances
  struct CurveEntry
  {
    Handle(Geom_Curve) MyCurve;       //!< The actual curve object
    int SubIndexCount;                 //!< Number of handle instances
  };

  //! Sequence of unique curves
  NCollection_Sequence<CurveEntry> myCurves;

  //! Deduplication map: curve hash -> index in myCurves
  std::unordered_map<Standard_Size, int> myHashToIndex;

  //! Instance counter for statistics
  int myInstanceCount;
};

#endif // _ODE_Curve3dFile_HeaderFile
