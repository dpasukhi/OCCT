// Created on: 1995-03-15
// Created by: Laurent PAINNOT
// Copyright (c) 1995-1999 Matra Datavision
// Copyright (c) 1999-2014 OPEN CASCADE SAS
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

#ifndef _BRep_PolygonOnTriangulation_HeaderFile
#define _BRep_PolygonOnTriangulation_HeaderFile

#include <Standard.hxx>
#include <Standard_Type.hxx>

#include <BRep_CurveRepresentation.hxx>
class Poly_PolygonOnTriangulation;
class Poly_Triangulation;
class TopLoc_Location;

class BRep_PolygonOnTriangulation;
DEFINE_STANDARD_HANDLE(BRep_PolygonOnTriangulation, BRep_CurveRepresentation)

//! A representation by an array of nodes on a
//! triangulation.
class BRep_PolygonOnTriangulation : public BRep_CurveRepresentation
{

public:
  Standard_EXPORT BRep_PolygonOnTriangulation(const Handle(Poly_PolygonOnTriangulation)& P,
                                              const Handle(Poly_Triangulation)&          T,
                                              const TopLoc_Location&                     L);

  //! returns True.
  Standard_EXPORT virtual Standard_Boolean IsPolygonOnTriangulation() const override;

  //! Is it a polygon in the definition of <T> with
  //! location <L>.
  Standard_EXPORT virtual Standard_Boolean IsPolygonOnTriangulation(
    const Handle(Poly_Triangulation)& T,
    const TopLoc_Location&            L) const override;

  //! returns True.
  Standard_EXPORT virtual void PolygonOnTriangulation(const Handle(Poly_PolygonOnTriangulation)& P)
    override;

  Standard_EXPORT virtual const Handle(Poly_Triangulation)& Triangulation() const override;

  Standard_EXPORT virtual const Handle(Poly_PolygonOnTriangulation)& PolygonOnTriangulation() const
    override;

  //! Return a copy of this representation.
  Standard_EXPORT virtual Handle(BRep_CurveRepresentation) Copy() const override;

  //! Dumps the content of me into the stream
  Standard_EXPORT virtual void DumpJson(Standard_OStream& theOStream,
                                        Standard_Integer  theDepth = -1) const override;

  DEFINE_STANDARD_RTTIEXT(BRep_PolygonOnTriangulation, BRep_CurveRepresentation)

protected:
private:
  Handle(Poly_PolygonOnTriangulation) myPolygon;
  Handle(Poly_Triangulation)          myTriangulation;
};

#endif // _BRep_PolygonOnTriangulation_HeaderFile
