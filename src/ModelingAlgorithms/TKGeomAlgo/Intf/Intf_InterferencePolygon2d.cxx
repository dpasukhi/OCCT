// Created on: 1991-06-24
// Created by: Didier PIFFAULT
// Copyright (c) -1999 Matra Datavision
// Copyright (c) 1991-1999 Matra Datavision
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

#include <Intf_InterferencePolygon2d.hxx>
#include <Intf_Polygon2d.hxx>
#include <Intf_SectionPoint.hxx>
#include <NCollection_Sequence.hxx>
#include <Intf_TangentZone.hxx>
#include <Precision.hxx>
#include <Standard_Integer.hxx>
#include <NCollection_List.hxx>
#include <NCollection_Array1.hxx>

// Angular precision (sinus) below that value two right segments
// are considered as having a potential zone of tangency.
namespace
{
static const double PRCANG = Precision::Angular();

//! Geometry and conservative bounds of one polygon segment, reused for all pairs.
//! Storage is zero-based; only the polygon API uses one-based segment IDs.
struct SegmentData
{
  gp_Pnt2d  First;
  gp_Pnt2d  Last;
  Bnd_Box2d Box;
};

//=================================================================================================

static NCollection_Array1<SegmentData> polygonSegments(const Intf_Polygon2d& thePolygon)
{
  const size_t                    aNbSegments = static_cast<size_t>(thePolygon.NbSegments());
  NCollection_Array1<SegmentData> aSegments(aNbSegments);
  const double                    aDeflection = thePolygon.DeflectionOverEstimation();
  for (size_t i = 0; i < aSegments.Size(); ++i)
  {
    SegmentData& aSegment = aSegments.ChangeAt(i);
    thePolygon.Segment(static_cast<int>(i) + 1, aSegment.First, aSegment.Last);
    aSegment.Box.Add(aSegment.First);
    aSegment.Box.Add(aSegment.Last);
    aSegment.Box.Enlarge(aDeflection);
  }
  return aSegments;
}
} // namespace

//=================================================================================================

Intf_InterferencePolygon2d::Intf_InterferencePolygon2d()
    : Intf_Interference(false),
      oClos(false),
      tClos(false),
      nbso(0)
{
}

//=======================================================================
// function : Intf_InterferencePolygon2d
// purpose  : Constructor of the interference between two Polygon.
//=======================================================================

Intf_InterferencePolygon2d::Intf_InterferencePolygon2d(const Intf_Polygon2d& Obje1,
                                                       const Intf_Polygon2d& Obje2)
    : Intf_Interference(false),
      oClos(false),
      tClos(false),
      nbso(0)
{
  if (!Obje1.Bounding().IsOut(Obje2.Bounding()))
  {
    Tolerance = Obje1.DeflectionOverEstimation() + Obje2.DeflectionOverEstimation();
    if (Tolerance == 0.)
    {
      Tolerance = Epsilon(1000.);
    }
    nbso  = Obje1.NbSegments();
    oClos = Obje1.Closed();
    tClos = Obje2.Closed();
    Interference(Obje1, Obje2);
    Clean();
  }
}

//=======================================================================
// function : Intf_InterferencePolygon2d
// purpose  : Constructor of the auto interference of a Polygon.
//=======================================================================

Intf_InterferencePolygon2d::Intf_InterferencePolygon2d(const Intf_Polygon2d& Obje)
    : Intf_Interference(true),
      oClos(false),
      tClos(false),
      nbso(0)
{
  Tolerance = Obje.DeflectionOverEstimation() * 2;
  if (Tolerance == 0.)
  {
    Tolerance = Epsilon(1000.);
  }
  oClos = Obje.Closed();
  tClos = oClos;
  Interference(Obje);
  Clean();
}

//=================================================================================================

void Intf_InterferencePolygon2d::Perform(const Intf_Polygon2d& Obje1, const Intf_Polygon2d& Obje2)
{
  SelfInterference(false);
  if (!Obje1.Bounding().IsOut(Obje2.Bounding()))
  {
    Tolerance = Obje1.DeflectionOverEstimation() + Obje2.DeflectionOverEstimation();
    if (Tolerance == 0.)
    {
      Tolerance = Epsilon(1000.);
    }
    nbso  = Obje1.NbSegments();
    oClos = Obje1.Closed();
    tClos = Obje2.Closed();
    Interference(Obje1, Obje2);
    Clean();
  }
}

//=================================================================================================

void Intf_InterferencePolygon2d::Perform(const Intf_Polygon2d& Obje)
{
  SelfInterference(true);
  Tolerance = Obje.DeflectionOverEstimation() * 2;
  if (Tolerance == 0.)
  {
    Tolerance = Epsilon(1000.);
  }
  oClos = Obje.Closed();
  tClos = oClos;
  Interference(Obje);
  Clean();
}

//=======================================================================
// function : Pnt2dValue
// purpose  : Give the section point of range Index in the interference.
//=======================================================================

gp_Pnt2d Intf_InterferencePolygon2d::Pnt2dValue(const int Index) const
{
  return gp_Pnt2d((mySPoins(Index)).Pnt().X(), (mySPoins(Index)).Pnt().Y());
}

//=================================================================================================

void Intf_InterferencePolygon2d::Interference(const Intf_Polygon2d& Obje1,
                                              const Intf_Polygon2d& Obje2)
{
  // The inner polygon is traversed once per outer segment. Cache its geometry
  // and boxes once, preserving the original segment-pair order and predicates.
  const NCollection_Array1<SegmentData> aSegments   = polygonSegments(Obje2);
  const double                          aDeflection = Obje1.DeflectionOverEstimation();
  Bnd_Box2d                             aBox;
  gp_Pnt2d                              aFirst, aLast;
  for (size_t i = 0; i < static_cast<size_t>(nbso); ++i)
  {
    aBox.SetVoid();
    Obje1.Segment(static_cast<int>(i) + 1, aFirst, aLast);
    aBox.Add(aFirst);
    aBox.Add(aLast);
    aBox.Enlarge(aDeflection);
    if (Obje2.Bounding().IsOut(aBox))
    {
      continue;
    }
    for (size_t j = 0; j < aSegments.Size(); ++j)
    {
      const SegmentData& aSegment = aSegments.At(j);
      if (!aBox.IsOut(aSegment.Box))
      {
        Intersect(static_cast<int>(i) + 1,
                  static_cast<int>(j) + 1,
                  aFirst,
                  aLast,
                  aSegment.First,
                  aSegment.Last);
      }
    }
  }
}

//=================================================================================================

void Intf_InterferencePolygon2d::Interference(const Intf_Polygon2d& Obje)
{
  // Both sides use the same records. Intersect() retains its existing handling
  // of adjacent segments and closed-polygon endpoints.
  const NCollection_Array1<SegmentData> aSegments = polygonSegments(Obje);
  nbso                                            = static_cast<int>(aSegments.Size());
  for (size_t i = 0; i < aSegments.Size(); ++i)
  {
    const SegmentData& aFirst = aSegments.At(i);
    if (Obje.Bounding().IsOut(aFirst.Box))
    {
      continue;
    }
    for (size_t j = i + 1; j < aSegments.Size(); ++j)
    {
      const SegmentData& aSecond = aSegments.At(j);
      if (!aFirst.Box.IsOut(aSecond.Box))
      {
        Intersect(static_cast<int>(i) + 1,
                  static_cast<int>(j) + 1,
                  aFirst.First,
                  aFirst.Last,
                  aSecond.First,
                  aSecond.Last);
      }
    }
  }
}

//=================================================================================================

void Intf_InterferencePolygon2d::Clean()
{

  // The zones of tangency that concerns only one couple of segments are
  // conserved if the angle between the segments is less than <PRCANG> and
  // if there is no real point of intersection EDGE/EDGE:
  int         nbIt  = myTZones.Length();
  int         decal = 0;
  int         addr1, addr2;
  Intf_PIType dim1, dim2;
  double      par;
  int         tsp, tsps;
  int         lpi, ltz;
  bool        Only1Seg = false;

#define PI1 (myTZones(ltz - decal).GetPoint(lpi))
#define PI2 (myTZones(ltz - decal).GetPoint(tsp))

  for (ltz = 1; ltz <= nbIt; ltz++)
  {
    tsp = tsps = 0;
    double pr1mi, pr1ma, pr2mi, pr2ma, delta1, delta2;
    myTZones(ltz - decal).ParamOnFirst(pr1mi, pr1ma);
    delta1 = pr1ma - pr1mi;
    myTZones(ltz - decal).ParamOnSecond(pr2mi, pr2ma);
    delta2 = pr2ma - pr2mi;
    if (delta1 < 1. && delta2 < 1.)
    {
      Only1Seg = true;
    }
    if (delta1 == 0. || delta2 == 0.)
    {
      Only1Seg = true;
    }

    for (lpi = 1; lpi <= myTZones(ltz - decal).NumberOfPoints(); lpi++)
    {
      if (PI1.Incidence() <= PRCANG)
      {
        tsp = tsps = 0;
        break;
      }
      PI1.InfoFirst(dim1, addr1, par);
      PI1.InfoSecond(dim2, addr2, par);
      if (dim1 == Intf_EDGE && dim2 == Intf_EDGE)
      {
        tsps = 0;
        if (tsp > 0)
        {
          tsp      = 0;
          Only1Seg = false;
          break;
        }
        tsp = lpi;
      }
      else if (dim1 != Intf_EXTERNAL && dim2 != Intf_EXTERNAL)
      {
        tsps = lpi;
      }
    }
    if (tsp > 0)
    {
      mySPoins.Append(myTZones(ltz - decal).GetPoint(tsp));
      myTZones.Remove(ltz - decal);
      decal++;
    }
    else if (Only1Seg && tsps != 0)
    {
      mySPoins.Append(myTZones(ltz - decal).GetPoint(tsps));
      myTZones.Remove(ltz - decal);
      decal++;
    }
  }

  // The points of intersection located in the tangency zone are
  // removed from the list :
  nbIt  = mySPoins.Length();
  decal = 0;

  for (lpi = 1; lpi <= nbIt; lpi++)
  {
    for (ltz = 1; ltz <= myTZones.Length(); ltz++)
    {
      if (myTZones(ltz).RangeContains(mySPoins(lpi - decal)))
      {
        mySPoins.Remove(lpi - decal);
        decal++;
        break;
      }
    }
  }
}

//=================================================================================================

void Intf_InterferencePolygon2d::Intersect(const int       iObje1,
                                           const int       iObje2,
                                           const gp_Pnt2d& BegO,
                                           const gp_Pnt2d& EndO,
                                           const gp_Pnt2d& BegT,
                                           const gp_Pnt2d& EndT)
{
  if (SelfIntf)
  {
    if (std::abs(iObje1 - iObje2) <= 1)
    {
      return; //-- Ajout du 15 jan 98
    }
  }

  int                                     nbpi = 0;
  double                                  parO[8];
  double                                  parT[8];
  NCollection_Sequence<Intf_SectionPoint> thePi;
  gp_XY                                   segT = EndT.XY() - BegT.XY();
  gp_XY                                   segO = EndO.XY() - BegO.XY();

  // If the length of segment is zero, nothing is done
  double lgT = std::sqrt(segT * segT);
  if (lgT <= 0.)
  {
    return;
  }
  double lgO = std::sqrt(segO * segO);
  if (lgO <= 0.)
  {
    return;
  }

  // Direction of parsing of segments
  double sigPS = (segO * segT) > 0.0 ? 1.0 : -1.0;

  // Precision of calculation
  double floatgap = Epsilon(lgO + lgT);

  // Angle between two straight lines and radius of interference
  double sinTeta = (segO.CrossMagnitude(segT) / lgO) / lgT;
  double rayIntf = 0.;
  if (sinTeta > 0.)
  {
    rayIntf = Tolerance / sinTeta;
  }

  // Interference <begO> <segT>
  double dbOT  = ((BegO.XY() - BegT.XY()) ^ segT) / lgT;
  double dbObT = BegO.Distance(BegT);
  double dbOeT = BegO.Distance(EndT);
  if (std::abs(dbOT) <= Tolerance)
  {
    if (dbObT <= Tolerance)
    {
      nbpi++;
      parO[nbpi] = 0.;
      parT[nbpi] = 0.;
      thePi.Append(
        Intf_SectionPoint(BegO, Intf_VERTEX, iObje1, 0., Intf_VERTEX, iObje2, 0., sinTeta));
    }
    if (dbOeT <= Tolerance)
    {
      nbpi++;
      parO[nbpi] = 0.;
      parT[nbpi] = 1.;
      thePi.Append(
        Intf_SectionPoint(BegO, Intf_VERTEX, iObje1, 0., Intf_VERTEX, iObje2 + 1, 0., sinTeta));
    }
    if (dbObT > Tolerance && dbOeT > Tolerance && dbObT + dbOeT <= (lgT + Tolerance))
    {
      nbpi++;
      parO[nbpi] = 0.;
      parT[nbpi] = dbObT / lgT;
      thePi.Append(
        Intf_SectionPoint(BegO, Intf_VERTEX, iObje1, 0., Intf_EDGE, iObje2, parT[nbpi], sinTeta));
    }
  }

  // Interference <endO> <segT>
  double deOT  = ((EndO.XY() - BegT.XY()) ^ segT) / lgT;
  double deObT = EndO.Distance(BegT);
  double deOeT = EndO.Distance(EndT);
  if (std::abs(deOT) <= Tolerance)
  {
    if (deObT <= Tolerance)
    {
      nbpi++;
      parO[nbpi] = 1.;
      parT[nbpi] = 0.;
      thePi.Append(
        Intf_SectionPoint(EndO, Intf_VERTEX, iObje1 + 1, 0., Intf_VERTEX, iObje2, 0., sinTeta));
    }
    if (deOeT <= Tolerance)
    {
      nbpi++;
      parO[nbpi] = 1.;
      parT[nbpi] = 1.;
      thePi.Append(
        Intf_SectionPoint(EndO, Intf_VERTEX, iObje1 + 1, 0., Intf_VERTEX, iObje2 + 1, 0., sinTeta));
    }
    if (deObT > Tolerance && deOeT > Tolerance && deObT + deOeT <= (lgT + Tolerance))
    {
      nbpi++;
      parO[nbpi] = 1.;
      parT[nbpi] = deObT / lgT;
      thePi.Append(Intf_SectionPoint(EndO,
                                     Intf_VERTEX,
                                     iObje1 + 1,
                                     0.,
                                     Intf_EDGE,
                                     iObje2,
                                     parT[nbpi],
                                     sinTeta));
    }
  }

  // Interference <begT> <segO>
  double dbTO = ((BegT.XY() - BegO.XY()) ^ segO) / lgO;
  if (std::abs(dbTO) <= Tolerance)
  {
    if (dbObT > Tolerance && deObT > Tolerance && dbObT + deObT <= (lgO + Tolerance))
    {
      nbpi++;
      parO[nbpi] = dbObT / lgO;
      parT[nbpi] = 0.;
      thePi.Append(
        Intf_SectionPoint(BegT, Intf_EDGE, iObje1, parO[nbpi], Intf_VERTEX, iObje2, 0., sinTeta));
    }
  }

  // Interference <endT> <segO>
  double deTO = ((EndT.XY() - BegO.XY()) ^ segO) / lgO;
  if (std::abs(deTO) <= Tolerance)
  {
    if (dbOeT > Tolerance && deOeT > Tolerance && dbOeT + deOeT <= (lgO + Tolerance))
    {
      nbpi++;
      parO[nbpi] = dbOeT / lgO;
      parT[nbpi] = 1.;
      thePi.Append(Intf_SectionPoint(EndT,
                                     Intf_EDGE,
                                     iObje1,
                                     parO[nbpi],
                                     Intf_VERTEX,
                                     iObje2 + 1,
                                     0.,
                                     sinTeta));
    }
  }

  bool   edgeSP = false;
  double parOSP = 0, parTSP = 0;

  if (std::abs(dbOT - deOT) > floatgap && std::abs(dbTO - deTO) > floatgap)
  {
    parOSP = dbOT / (dbOT - deOT);
    parTSP = dbTO / (dbTO - deTO);
    if (dbOT * deOT <= 0. && dbTO * deTO <= 0.)
    {
      edgeSP = true;
    }
    else if (nbpi == 0)
    {
      return;
    }

    // If there is no interference it is necessary to take the points segment by segment
    if (nbpi == 0 && sinTeta > PRCANG)
    {
      nbpi++;
      parO[nbpi] = parOSP;
      parT[nbpi] = parTSP;
      thePi.Append(
        Intf_SectionPoint(gp_Pnt2d(BegO.X() + (segO.X() * parOSP), BegO.Y() + (segO.Y() * parOSP)),
                          Intf_EDGE,
                          iObje1,
                          parOSP,
                          Intf_EDGE,
                          iObje2,
                          parTSP,
                          sinTeta));
    }

    // Otherwise it is required to check if there is no other
    else if (rayIntf >= Tolerance)
    {
      double deltaO = rayIntf / lgO;
      double deltaT = rayIntf / lgT;
      double x, y;
      double parOdeb = parOSP - deltaO;
      double parOfin = parOSP + deltaO;
      double parTdeb = parTSP - sigPS * deltaT;
      double parTfin = parTSP + sigPS * deltaT;
      if (nbpi == 0)
      {
        parO[1] = parOdeb;
        parO[2] = parOfin;
        parT[1] = parTdeb;
        parT[2] = parTfin;
        while (nbpi < 2)
        {
          nbpi++;
          x = BegO.X() + (segO.X() * parO[nbpi]);
          y = BegO.Y() + (segO.Y() * parO[nbpi]);
          thePi.Append(Intf_SectionPoint(gp_Pnt2d(x, y),
                                         Intf_EXTERNAL,
                                         iObje1,
                                         parO[nbpi],
                                         Intf_EXTERNAL,
                                         iObje2,
                                         parT[nbpi],
                                         sinTeta));
        }
      }
      else
      { // nbpi>0
        if (nbpi == 1)
        {
          bool ok = true;
          if (0. < parOdeb && parOdeb < 1. && 0. < parTdeb && parTdeb < 1.)
          {
            parO[nbpi + 1] = parOdeb;
            parT[nbpi + 1] = parTdeb;
          }
          else if (0. < parOfin && parOfin < 1. && 0. < parTfin && parTfin < 1.)
          {
            parO[nbpi + 1] = parOfin;
            parT[nbpi + 1] = parTfin;
          }
          else
          {
            ok = false;
          }

          if (ok)
          {
            x = BegO.X() + (segO.X() * parO[nbpi + 1]);
            y = BegO.Y() + (segO.Y() * parO[nbpi + 1]);
            if (thePi(1).Pnt().Distance(gp_Pnt(x, y, 0)) >= (Tolerance / 4.))
            {
              nbpi++;
              thePi.Append(Intf_SectionPoint(gp_Pnt2d(x, y),
                                             Intf_EXTERNAL,
                                             iObje1,
                                             parO[nbpi],
                                             Intf_EXTERNAL,
                                             iObje2,
                                             parT[nbpi],
                                             sinTeta));
            }
          }
        }
        else
        { // plus d une singularite
          double parOmin = parO[1];
          double parOmax = parO[1];
          double parTmin = parT[1];
          double parTmax = parT[1];
          for (int i = 2; i <= nbpi; i++)
          {
            parOmin = std::min(parOmin, parO[i]);
            parOmax = std::max(parOmax, parO[i]);
            parTmin = std::min(parTmin, parT[i]);
            parTmax = std::max(parTmax, parT[i]);
          }

          double delta;
          if (parOdeb < 0.)
          {
            delta   = -parOdeb;
            parOdeb = 0.;
            parTdeb = parTdeb + sigPS * (delta * (deltaT / deltaO));
          }
          if (parOfin > 1.)
          {
            delta   = parOfin - 1.;
            parOfin = 1.;
            parTfin = parTfin - sigPS * (delta * (deltaT / deltaO));
          }
          if (sigPS > 0.)
          {
            if (parTdeb < 0.)
            {
              delta   = -parTdeb;
              parTdeb = 0.;
              parOdeb = parOdeb + delta * (deltaO / deltaT);
            }
            if (parTfin > 1.)
            {
              delta   = parTfin - 1.;
              parTfin = 1.;
              parOfin = parOfin - delta * (deltaO / deltaT);
            }
          }
          else
          {
            if (parTdeb > 1.)
            {
              delta   = parTdeb - 1.;
              parTdeb = 1.;
              parOdeb = parOdeb + delta * (deltaO / deltaT);
            }
            if (parTfin < 0.)
            {
              delta   = -parTfin;
              parTfin = 0.;
              parOfin = parOfin - delta * (deltaO / deltaT);
            }
          }

          if ((parOdeb < parOmin && parOmin > 0.)
              || (sigPS > 0. && parTdeb < parTmin && parTmin > 0.)
              || (sigPS < 0. && parTdeb > parTmax && parTmax < 1.))
          {
            nbpi++;
            parO[nbpi] = std::max(0., std::min(1., parOdeb));
            parT[nbpi] = std::max(0., std::min(1., parTdeb));
            x          = BegO.X() + (segO.X() * parO[nbpi]);
            y          = BegO.Y() + (segO.Y() * parO[nbpi]);
            thePi.Append(Intf_SectionPoint(gp_Pnt2d(x, y),
                                           Intf_EXTERNAL,
                                           iObje1,
                                           parO[nbpi],
                                           Intf_EXTERNAL,
                                           iObje2,
                                           parT[nbpi],
                                           sinTeta));
          }

          if ((parOfin > parOmax && parOmax < 1.)
              || (sigPS < 0. && parTfin < parTmin && parTmin > 0.)
              || (sigPS > 0. && parTfin > parTmax && parTmax < 1.))
          {
            nbpi++;
            parO[nbpi] = std::min(1., std::max(0., parOfin));
            parT[nbpi] = std::min(1., std::max(0., parTfin));
            x          = BegO.X() + (segO.X() * parO[nbpi]);
            y          = BegO.Y() + (segO.Y() * parO[nbpi]);
            thePi.Append(Intf_SectionPoint(gp_Pnt2d(x, y),
                                           Intf_EXTERNAL,
                                           iObje1,
                                           parO[nbpi],
                                           Intf_EXTERNAL,
                                           iObje2,
                                           parT[nbpi],
                                           sinTeta));
          }
        }
      }
    }
  }

  //-- lbr : The points too close to each other are suspended
  bool suppr;
  do
  {
    suppr = false;
    for (int i = 2; !suppr && i <= nbpi; i++)
    {
      const gp_Pnt& Pim1 = thePi(i - 1).Pnt();
      const gp_Pnt& Pi   = thePi(i).Pnt();
      double        d    = Pi.Distance(Pim1);
      d *= 50.0;
      if (d < lgT && d < lgO)
      {
        for (int j = i; j < nbpi; j++)
        {
          thePi(j) = thePi(j + 1);
        }
        nbpi--;
        suppr = true;
      }
    }
  } while (suppr);

  if (nbpi == 1)
  {
    if (edgeSP)
    {
      thePi(1) =
        Intf_SectionPoint(gp_Pnt2d(BegO.X() + (segO.X() * parOSP), BegO.Y() + (segO.Y() * parOSP)),
                          Intf_EDGE,
                          iObje1,
                          parOSP,
                          Intf_EDGE,
                          iObje2,
                          parTSP,
                          sinTeta);
      parO[1] = parOSP;
      parT[1] = parTSP;
    }
    if (!SelfIntf)
    {
      bool contains = false;
      for (int i = 1; i <= mySPoins.Length(); i++)
      {
        if (thePi(1).IsEqual(mySPoins(i)))
        {
          contains = true;
          break;
        }
      }
      if (!contains)
      {
        mySPoins.Append(thePi(1));
      }
    }
    // Only the first/last pair shares the closure vertex. Other pairs that
    // involve either end segment can contain genuine self-intersections.
    else if (iObje2 - iObje1 != 1 && (!oClos || iObje1 != 1 || iObje2 != nbso))
    {
      mySPoins.Append(thePi(1));
    }
  }

  else if (nbpi >= 2)
  {
    Intf_TangentZone TheTZ;
    if (nbpi == 2)
    {
      TheTZ.PolygonInsert(thePi(1));
      TheTZ.PolygonInsert(thePi(2));
    }
    else
    {
      int lpj;
      int lmin = 1;
      int lmax = 1;
      for (lpj = 2; lpj <= nbpi; lpj++)
      {
        if (parO[lpj] < parO[lmin])
        {
          lmin = lpj;
        }
        else if (parO[lpj] > parO[lmax])
        {
          lmax = lpj;
        }
      }
      TheTZ.PolygonInsert(thePi(lmin));
      TheTZ.PolygonInsert(thePi(lmax));

      int ltmin = 1;
      int ltmax = 1;
      for (lpj = 2; lpj <= nbpi; lpj++)
      {
        if (parT[lpj] < parT[ltmin])
        {
          ltmin = lpj;
        }
        else if (parT[lpj] > parT[ltmax])
        {
          ltmax = lpj;
        }
      }
      if (ltmin != lmin && ltmin != lmax)
      {
        TheTZ.PolygonInsert(thePi(ltmin));
      }
      if (ltmax != lmin && ltmax != lmax)
      {
        TheTZ.PolygonInsert(thePi(ltmax));
      }
    }

    if (edgeSP)
    {
      TheTZ.PolygonInsert(
        Intf_SectionPoint(gp_Pnt2d(BegO.X() + (segO.X() * parOSP), BegO.Y() + (segO.Y() * parOSP)),
                          Intf_EDGE,
                          iObje1,
                          parOSP,
                          Intf_EDGE,
                          iObje2,
                          parTSP,
                          sinTeta));
    }

    int                   nbtz = myTZones.Length();
    NCollection_List<int> LIndex;
    for (int ltz = 1; ltz <= nbtz; ltz++)
    {
      if (TheTZ.HasCommonRange(myTZones(ltz)))
      {
        LIndex.Append(ltz);
      }
    }
    //------------------------------------------------------------------------
    //--   The list is parsed in ascending order by index, zone and tg
    //--
    if (LIndex.IsEmpty())
    {
      myTZones.Append(TheTZ);
    }
    else
    {
      int indexfirst = LIndex.First();
      LIndex.RemoveFirst();
      int decal = 0;
      myTZones(indexfirst).Append(TheTZ);
      while (!LIndex.IsEmpty())
      {
        int index = LIndex.First();
        LIndex.RemoveFirst();
        myTZones(indexfirst).Append(myTZones(index - decal));
        myTZones.Remove(index - decal);
        decal++;
      }
    }
  }
}
