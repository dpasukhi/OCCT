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

#include <GeomHash_TriangulationHasher.hxx>

#include <Poly_Triangle.hxx>
#include <Poly_Triangulation.hxx>
#include <Standard_HashUtils.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>

#include <cmath>

GeomHash_TriangulationHasher::GeomHash_TriangulationHasher(const double theCompTolerance,
                                                           const double theHashTolerance)
    : CompTolerance(theCompTolerance),
      HashTolerance(theHashTolerance)
{
}

std::size_t GeomHash_TriangulationHasher::operator()(
  const occ::handle<Poly_Triangulation>& theTri) const noexcept
{
  const std::size_t aHashes[3] = {
    opencascade::hash(theTri->NbNodes()),
    opencascade::hash(theTri->NbTriangles()),
    opencascade::hash(theTri->HasUVNodes() ? 1 : 0)};

  return opencascade::hashBytes(aHashes, sizeof(aHashes));
}

bool GeomHash_TriangulationHasher::operator()(
  const occ::handle<Poly_Triangulation>& theTri1,
  const occ::handle<Poly_Triangulation>& theTri2) const noexcept
{
  if (theTri1->NbNodes() != theTri2->NbNodes())
  {
    return false;
  }
  if (theTri1->NbTriangles() != theTri2->NbTriangles())
  {
    return false;
  }
  if (theTri1->HasUVNodes() != theTri2->HasUVNodes())
  {
    return false;
  }
  if (std::abs(theTri1->Deflection() - theTri2->Deflection()) > CompTolerance)
  {
    return false;
  }

  for (int aIdx = 1; aIdx <= theTri1->NbNodes(); ++aIdx)
  {
    const gp_Pnt& aP1 = theTri1->Node(aIdx);
    const gp_Pnt& aP2 = theTri2->Node(aIdx);
    if (std::abs(aP1.X() - aP2.X()) > CompTolerance
        || std::abs(aP1.Y() - aP2.Y()) > CompTolerance
        || std::abs(aP1.Z() - aP2.Z()) > CompTolerance)
    {
      return false;
    }
  }

  for (int aIdx = 1; aIdx <= theTri1->NbTriangles(); ++aIdx)
  {
    int aN11 = 0, aN12 = 0, aN13 = 0;
    int aN21 = 0, aN22 = 0, aN23 = 0;
    theTri1->Triangle(aIdx).Get(aN11, aN12, aN13);
    theTri2->Triangle(aIdx).Get(aN21, aN22, aN23);
    if (aN11 != aN21 || aN12 != aN22 || aN13 != aN23)
    {
      return false;
    }
  }

  if (theTri1->HasUVNodes())
  {
    for (int aIdx = 1; aIdx <= theTri1->NbNodes(); ++aIdx)
    {
      const gp_Pnt2d& aU1 = theTri1->UVNode(aIdx);
      const gp_Pnt2d& aU2 = theTri2->UVNode(aIdx);
      if (std::abs(aU1.X() - aU2.X()) > CompTolerance
          || std::abs(aU1.Y() - aU2.Y()) > CompTolerance)
      {
        return false;
      }
    }
  }

  return true;
}
