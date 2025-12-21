// Created on: 1993-02-05
// Created by: Jacques GOUSSARD
// Copyright (c) 1993-1999 Matra Datavision
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

#ifndef _Contap_TheSearchInside_HeaderFile
#define _Contap_TheSearchInside_HeaderFile

#include <GeomAdaptor_Surface.hxx>
#include <IntSurf_SequenceOfInteriorPoint.hxx>

class GeomAdaptor_HSurfaceTool;
class GeomAdaptor_TopolTool;
class Contap_HContTool;
class Contap_SurfFunction;
class IntSurf_InteriorPoint;

class Contap_TheSearchInside
{
public:
  DEFINE_STANDARD_ALLOC

  Standard_EXPORT Contap_TheSearchInside();

  Standard_EXPORT Contap_TheSearchInside(Contap_SurfFunction&               F,
                                         const Handle(GeomAdaptor_Surface)&   Surf,
                                         const Handle(GeomAdaptor_TopolTool)& T,
                                         const Standard_Real                Epsilon);

  Standard_EXPORT void Perform(Contap_SurfFunction&               F,
                               const Handle(GeomAdaptor_Surface)&   Surf,
                               const Handle(GeomAdaptor_TopolTool)& T,
                               const Standard_Real                Epsilon);

  Standard_EXPORT void Perform(Contap_SurfFunction&             F,
                               const Handle(GeomAdaptor_Surface)& Surf,
                               const Standard_Real              UStart,
                               const Standard_Real              VStart);

  Standard_Boolean IsDone() const;

  //! Returns the number of points.
  //! The exception NotDone if raised if IsDone
  //! returns False.
  Standard_Integer NbPoints() const;

  //! Returns the point of range Index.
  //! The exception NotDone if raised if IsDone
  //! returns False.
  //! The exception OutOfRange if raised if
  //! Index <= 0 or Index > NbPoints.
  const IntSurf_InteriorPoint& Value(const Standard_Integer Index) const;

protected:
private:
  Standard_Boolean                done;
  IntSurf_SequenceOfInteriorPoint list;
};

#define ThePSurface Handle(GeomAdaptor_Surface)
#define ThePSurface_hxx <GeomAdaptor_Surface.hxx>
#define ThePSurfaceTool GeomAdaptor_HSurfaceTool
#define ThePSurfaceTool_hxx <GeomAdaptor_HSurfaceTool.hxx>
#define Handle_TheTopolTool Handle(GeomAdaptor_TopolTool)
#define TheTopolTool GeomAdaptor_TopolTool
#define TheTopolTool_hxx <GeomAdaptor_TopolTool.hxx>
#define TheSITool Contap_HContTool
#define TheSITool_hxx <Contap_HContTool.hxx>
#define TheFunction Contap_SurfFunction
#define TheFunction_hxx <Contap_SurfFunction.hxx>
#define IntStart_SearchInside Contap_TheSearchInside
#define IntStart_SearchInside_hxx <Contap_TheSearchInside.hxx>

#include <IntStart_SearchInside.lxx>

#undef ThePSurface
#undef ThePSurface_hxx
#undef ThePSurfaceTool
#undef ThePSurfaceTool_hxx
#undef Handle_TheTopolTool
#undef TheTopolTool
#undef TheTopolTool_hxx
#undef TheSITool
#undef TheSITool_hxx
#undef TheFunction
#undef TheFunction_hxx
#undef IntStart_SearchInside
#undef IntStart_SearchInside_hxx

#endif // _Contap_TheSearchInside_HeaderFile
