// Copyright (c) 2026 OPEN CASCADE SAS
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

#include <GeomHash_Polygon3DHasher.hxx>

#include <NCollection_Array1.hxx>
#include <Poly_Polygon3D.hxx>
#include <Standard_HashUtils.hxx>
#include <gp_Pnt.hxx>

#include <cmath>

GeomHash_Polygon3DHasher::GeomHash_Polygon3DHasher(const double theCompTolerance,
                                                   const double theHashTolerance)
    : CompTolerance(theCompTolerance),
      HashTolerance(theHashTolerance)
{
}

std::size_t GeomHash_Polygon3DHasher::operator()(
  const occ::handle<Poly_Polygon3D>& thePoly) const noexcept
{
  const std::size_t aHashes[2] = {opencascade::hash(thePoly->NbNodes()),
                                  opencascade::hash(thePoly->HasParameters() ? 1 : 0)};

  return opencascade::hashBytes(aHashes, sizeof(aHashes));
}

bool GeomHash_Polygon3DHasher::operator()(
  const occ::handle<Poly_Polygon3D>& thePoly1,
  const occ::handle<Poly_Polygon3D>& thePoly2) const noexcept
{
  if (thePoly1->NbNodes() != thePoly2->NbNodes())
  {
    return false;
  }
  if (thePoly1->HasParameters() != thePoly2->HasParameters())
  {
    return false;
  }
  if (std::abs(thePoly1->Deflection() - thePoly2->Deflection()) > CompTolerance)
  {
    return false;
  }

  const NCollection_Array1<gp_Pnt>& aNodes1 = thePoly1->Nodes();
  const NCollection_Array1<gp_Pnt>& aNodes2 = thePoly2->Nodes();
  for (int aIdx = aNodes1.Lower(); aIdx <= aNodes1.Upper(); ++aIdx)
  {
    const gp_Pnt& aP1 = aNodes1.Value(aIdx);
    const gp_Pnt& aP2 = aNodes2.Value(aIdx);
    if (std::abs(aP1.X() - aP2.X()) > CompTolerance || std::abs(aP1.Y() - aP2.Y()) > CompTolerance
        || std::abs(aP1.Z() - aP2.Z()) > CompTolerance)
    {
      return false;
    }
  }

  if (thePoly1->HasParameters())
  {
    const NCollection_Array1<double>& aParams1 = thePoly1->Parameters();
    const NCollection_Array1<double>& aParams2 = thePoly2->Parameters();
    for (int aIdx = aParams1.Lower(); aIdx <= aParams1.Upper(); ++aIdx)
    {
      if (std::abs(aParams1.Value(aIdx) - aParams2.Value(aIdx)) > CompTolerance)
      {
        return false;
      }
    }
  }

  return true;
}
