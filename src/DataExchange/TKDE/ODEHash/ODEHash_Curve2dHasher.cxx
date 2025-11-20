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

#include <ODEHash_Curve2dHasher.hxx>

#include <Standard_HashUtils.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom2d_Line.hxx>
#include <Geom2d_Circle.hxx>
#include <Geom2d_Ellipse.hxx>
#include <Geom2d_Hyperbola.hxx>
#include <Geom2d_Parabola.hxx>
#include <Geom2d_BezierCurve.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <Geom2d_OffsetCurve.hxx>

#include <ODEHash_Line2dHasher.pxx>
#include <ODEHash_Circle2dHasher.pxx>
#include <ODEHash_Ellipse2dHasher.pxx>
#include <ODEHash_Hyperbola2dHasher.pxx>
#include <ODEHash_Parabola2dHasher.pxx>
#include <ODEHash_BezierCurve2dHasher.pxx>
#include <ODEHash_BSplineCurve2dHasher.pxx>
#include <ODEHash_TrimmedCurve2dHasher.pxx>
#include <ODEHash_OffsetCurve2dHasher.pxx>

//=================================================================================================

std::size_t ODEHash_Curve2dHasher::operator()(const Handle(Geom2d_Curve)& theCurve) const noexcept
{
  if (theCurve.IsNull())
  {
    return 0;
  }

  // Dispatch based on actual curve type
  if (Handle(Geom2d_Line) aLine = Handle(Geom2d_Line)::DownCast(theCurve))
  {
    return ODEHash_Line2dHasher{}(aLine);
  }
  if (Handle(Geom2d_Circle) aCircle = Handle(Geom2d_Circle)::DownCast(theCurve))
  {
    return ODEHash_Circle2dHasher{}(aCircle);
  }
  if (Handle(Geom2d_Ellipse) anEllipse = Handle(Geom2d_Ellipse)::DownCast(theCurve))
  {
    return ODEHash_Ellipse2dHasher{}(anEllipse);
  }
  if (Handle(Geom2d_Hyperbola) aHyperbola = Handle(Geom2d_Hyperbola)::DownCast(theCurve))
  {
    return ODEHash_Hyperbola2dHasher{}(aHyperbola);
  }
  if (Handle(Geom2d_Parabola) aParabola = Handle(Geom2d_Parabola)::DownCast(theCurve))
  {
    return ODEHash_Parabola2dHasher{}(aParabola);
  }
  if (Handle(Geom2d_BezierCurve) aBezier = Handle(Geom2d_BezierCurve)::DownCast(theCurve))
  {
    return ODEHash_BezierCurve2dHasher{}(aBezier);
  }
  if (Handle(Geom2d_BSplineCurve) aBSpline = Handle(Geom2d_BSplineCurve)::DownCast(theCurve))
  {
    return ODEHash_BSplineCurve2dHasher{}(aBSpline);
  }
  if (Handle(Geom2d_TrimmedCurve) aTrimmed = Handle(Geom2d_TrimmedCurve)::DownCast(theCurve))
  {
    return ODEHash_TrimmedCurve2dHasher{}(aTrimmed);
  }
  if (Handle(Geom2d_OffsetCurve) anOffset = Handle(Geom2d_OffsetCurve)::DownCast(theCurve))
  {
    return ODEHash_OffsetCurve2dHasher{}(anOffset);
  }

  // Unknown curve type - hash the type name
  return std::hash<std::string>{}(theCurve->DynamicType()->Name());
}

//=================================================================================================

bool ODEHash_Curve2dHasher::operator()(const Handle(Geom2d_Curve)& theCurve1,
                                       const Handle(Geom2d_Curve)& theCurve2) const noexcept
{
  if (theCurve1.IsNull() || theCurve2.IsNull())
  {
    return theCurve1.IsNull() && theCurve2.IsNull();
  }

  if (theCurve1 == theCurve2)
  {
    return true;
  }

  // Must be same type
  if (theCurve1->DynamicType() != theCurve2->DynamicType())
  {
    return false;
  }

  // Dispatch based on actual curve type
  if (Handle(Geom2d_Line) aLine1 = Handle(Geom2d_Line)::DownCast(theCurve1))
  {
    return ODEHash_Line2dHasher{}(aLine1, Handle(Geom2d_Line)::DownCast(theCurve2));
  }
  if (Handle(Geom2d_Circle) aCircle1 = Handle(Geom2d_Circle)::DownCast(theCurve1))
  {
    return ODEHash_Circle2dHasher{}(aCircle1, Handle(Geom2d_Circle)::DownCast(theCurve2));
  }
  if (Handle(Geom2d_Ellipse) anEllipse1 = Handle(Geom2d_Ellipse)::DownCast(theCurve1))
  {
    return ODEHash_Ellipse2dHasher{}(anEllipse1, Handle(Geom2d_Ellipse)::DownCast(theCurve2));
  }
  if (Handle(Geom2d_Hyperbola) aHyp1 = Handle(Geom2d_Hyperbola)::DownCast(theCurve1))
  {
    return ODEHash_Hyperbola2dHasher{}(aHyp1, Handle(Geom2d_Hyperbola)::DownCast(theCurve2));
  }
  if (Handle(Geom2d_Parabola) aPar1 = Handle(Geom2d_Parabola)::DownCast(theCurve1))
  {
    return ODEHash_Parabola2dHasher{}(aPar1, Handle(Geom2d_Parabola)::DownCast(theCurve2));
  }
  if (Handle(Geom2d_BezierCurve) aBez1 = Handle(Geom2d_BezierCurve)::DownCast(theCurve1))
  {
    return ODEHash_BezierCurve2dHasher{}(aBez1, Handle(Geom2d_BezierCurve)::DownCast(theCurve2));
  }
  if (Handle(Geom2d_BSplineCurve) aBSpl1 = Handle(Geom2d_BSplineCurve)::DownCast(theCurve1))
  {
    return ODEHash_BSplineCurve2dHasher{}(aBSpl1, Handle(Geom2d_BSplineCurve)::DownCast(theCurve2));
  }
  if (Handle(Geom2d_TrimmedCurve) aTrim1 = Handle(Geom2d_TrimmedCurve)::DownCast(theCurve1))
  {
    return ODEHash_TrimmedCurve2dHasher{}(aTrim1, Handle(Geom2d_TrimmedCurve)::DownCast(theCurve2));
  }
  if (Handle(Geom2d_OffsetCurve) aOff1 = Handle(Geom2d_OffsetCurve)::DownCast(theCurve1))
  {
    return ODEHash_OffsetCurve2dHasher{}(aOff1, Handle(Geom2d_OffsetCurve)::DownCast(theCurve2));
  }

  // Unknown curve type - compare by pointer
  return theCurve1.get() == theCurve2.get();
}
