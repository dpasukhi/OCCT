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

#include <ODE_Curve2dFile.hxx>
#include <ODEHash_Curve2dHasher.hxx>

// Geom2d curve types
#include <Geom2d_Line.hxx>
#include <Geom2d_Circle.hxx>
#include <Geom2d_Ellipse.hxx>
#include <Geom2d_Hyperbola.hxx>
#include <Geom2d_Parabola.hxx>
#include <Geom2d_BezierCurve.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <Geom2d_OffsetCurve.hxx>

// Cap'n Proto includes
#include <capnp/message.h>
#include <capnp/serialize.h>
#include <curves2d.capnp.h>

// POSIX file I/O for Cap'n Proto
#include <fcntl.h>
#include <unistd.h>

IMPLEMENT_STANDARD_RTTIEXT(ODE_Curve2dFile, Standard_Transient)

//=================================================================================================

ODE_Curve2dFile::ODE_Curve2dFile()
: myInstanceCount(0)
{
}

//=================================================================================================

ODE_ObjectRef ODE_Curve2dFile::AddCurve(const Handle(Geom2d_Curve)& theCurve)
{
  if (theCurve.IsNull())
  {
    return ODE_ObjectRef();
  }

  // Compute hash using polymorphic hasher
  const ODEHash_Curve2dHasher aHasher;
  const Standard_Size aHash = aHasher(theCurve);

  // Check if curve already exists
  const auto it = myHashToIndex.find(aHash);
  if (it != myHashToIndex.end())
  {
    // Found duplicate - check for exact equality
    const int anIndex = it->second;
    CurveEntry& anEntry = myCurves.ChangeValue(anIndex);

    if (aHasher(anEntry.MyCurve, theCurve))
    {
      // Exact match - increment sub-index counter
      anEntry.SubIndexCount++;
      myInstanceCount++;

      // Return reference with sub-index
      return ODE_ObjectRef("curves2d", anIndex, anEntry.SubIndexCount - 1);
    }
  }

  // No duplicate found - add new curve
  CurveEntry aNewEntry;
  aNewEntry.MyCurve = theCurve;
  aNewEntry.SubIndexCount = 1;

  myCurves.Append(aNewEntry);
  const int aNewIndex = myCurves.Size();
  myHashToIndex[aHash] = aNewIndex;
  myInstanceCount++;

  // Return reference without sub-index (first instance)
  return ODE_ObjectRef("curves2d", aNewIndex);
}

//=================================================================================================

Handle(Geom2d_Curve) ODE_Curve2dFile::GetCurve(const ODE_ObjectRef& theRef) const
{
  if (theRef.FileType() != "curves2d")
  {
    return nullptr;
  }

  const int anIndex = theRef.Index();
  if (anIndex < 1 || anIndex > myCurves.Size())
  {
    return nullptr;
  }

  const CurveEntry& anEntry = myCurves.Value(anIndex);

  // In OCCT, Handle sharing is automatic
  return anEntry.MyCurve;
}

//=================================================================================================

int ODE_Curve2dFile::CurveCount() const
{
  return myCurves.Size();
}

//=================================================================================================

int ODE_Curve2dFile::InstanceCount() const
{
  return myInstanceCount;
}

//=================================================================================================

void ODE_Curve2dFile::Clear()
{
  myCurves.Clear();
  myHashToIndex.clear();
  myInstanceCount = 0;
}

//=================================================================================================

namespace
{
  // Helper to set Vec2
  void SetVec2(Vec2::Builder theBuilder, const gp_Pnt2d& thePnt)
  {
    theBuilder.setX(thePnt.X());
    theBuilder.setY(thePnt.Y());
  }

  void SetVec2(Vec2::Builder theBuilder, const gp_Dir2d& theDir)
  {
    theBuilder.setX(theDir.X());
    theBuilder.setY(theDir.Y());
  }

  // Helper to set AxisPlacement2d
  void SetAxisPlacement2d(AxisPlacement2d::Builder theBuilder, const gp_Ax22d& theAx)
  {
    SetVec2(theBuilder.initLocation(), theAx.Location());
    SetVec2(theBuilder.initRefDirection(), theAx.XDirection());
  }

  // Helper to get gp_Pnt2d from Vec2
  gp_Pnt2d GetPnt2d(const Vec2::Reader& theVec)
  {
    return gp_Pnt2d(theVec.getX(), theVec.getY());
  }

  // Helper to get gp_Dir2d from Vec2
  gp_Dir2d GetDir2d(const Vec2::Reader& theVec)
  {
    return gp_Dir2d(theVec.getX(), theVec.getY());
  }

  // Helper to get gp_Ax22d from AxisPlacement2d
  gp_Ax22d GetAxis2d(const AxisPlacement2d::Reader& theAxis)
  {
    return gp_Ax22d(GetPnt2d(theAxis.getLocation()), GetDir2d(theAxis.getRefDirection()));
  }
}

//=================================================================================================

ODE_Status ODE_Curve2dFile::WriteToFile(const TCollection_AsciiString& thePath) const
{
  capnp::MallocMessageBuilder aMessage;
  auto aFileBuilder = aMessage.initRoot<Curve2dFile>();
  auto aCurvesBuilder = aFileBuilder.initCurves(myCurves.Size());

  for (int i = 1; i <= myCurves.Size(); ++i)
  {
    const Handle(Geom2d_Curve)& aCurve = myCurves.Value(i).MyCurve;
    auto aCurveBuilder = aCurvesBuilder[i - 1];
    aCurveBuilder.setIndex(i);

    // Dispatch by curve type
    if (Handle(Geom2d_Line) aLine = Handle(Geom2d_Line)::DownCast(aCurve))
    {
      auto aBuilder = aCurveBuilder.initLine();
      SetVec2(aBuilder.initLocation(), aLine->Position().Location());
      SetVec2(aBuilder.initDirection(), aLine->Position().Direction());
    }
    else if (Handle(Geom2d_Circle) aCircle = Handle(Geom2d_Circle)::DownCast(aCurve))
    {
      auto aBuilder = aCurveBuilder.initCircle();
      SetAxisPlacement2d(aBuilder.initPosition(), aCircle->Position());
      aBuilder.setRadius(aCircle->Radius());
    }
    else if (Handle(Geom2d_Ellipse) anEllipse = Handle(Geom2d_Ellipse)::DownCast(aCurve))
    {
      auto aBuilder = aCurveBuilder.initEllipse();
      SetAxisPlacement2d(aBuilder.initPosition(), anEllipse->Position());
      aBuilder.setMajorRadius(anEllipse->MajorRadius());
      aBuilder.setMinorRadius(anEllipse->MinorRadius());
    }
    else if (Handle(Geom2d_Hyperbola) aHyperbola = Handle(Geom2d_Hyperbola)::DownCast(aCurve))
    {
      auto aBuilder = aCurveBuilder.initHyperbola();
      SetAxisPlacement2d(aBuilder.initPosition(), aHyperbola->Position());
      aBuilder.setMajorRadius(aHyperbola->MajorRadius());
      aBuilder.setMinorRadius(aHyperbola->MinorRadius());
    }
    else if (Handle(Geom2d_Parabola) aParabola = Handle(Geom2d_Parabola)::DownCast(aCurve))
    {
      auto aBuilder = aCurveBuilder.initParabola();
      SetAxisPlacement2d(aBuilder.initPosition(), aParabola->Position());
      aBuilder.setFocalLength(aParabola->Focal());
    }
    else if (Handle(Geom2d_BezierCurve) aBezier = Handle(Geom2d_BezierCurve)::DownCast(aCurve))
    {
      auto aBuilder = aCurveBuilder.initBezierCurve();
      aBuilder.setDegree(aBezier->Degree());
      aBuilder.setRational(aBezier->IsRational());
      aBuilder.setPeriodic(aBezier->IsPeriodic());

      auto aPolesBuilder = aBuilder.initPoles(aBezier->NbPoles());
      for (int j = 1; j <= aBezier->NbPoles(); ++j)
      {
        SetVec2(aPolesBuilder[j - 1], aBezier->Pole(j));
      }

      if (aBezier->IsRational())
      {
        auto aWeightsBuilder = aBuilder.initWeights(aBezier->NbPoles());
        for (int j = 1; j <= aBezier->NbPoles(); ++j)
        {
          aWeightsBuilder.set(j - 1, aBezier->Weight(j));
        }
      }
    }
    else if (Handle(Geom2d_BSplineCurve) aBSpline = Handle(Geom2d_BSplineCurve)::DownCast(aCurve))
    {
      auto aBuilder = aCurveBuilder.initBsplineCurve();
      aBuilder.setDegree(aBSpline->Degree());
      aBuilder.setRational(aBSpline->IsRational());
      aBuilder.setPeriodic(aBSpline->IsPeriodic());
      aBuilder.setClosed(aBSpline->IsClosed());

      auto aKnotsBuilder = aBuilder.initKnots(aBSpline->NbKnots());
      auto aMultsBuilder = aBuilder.initMultiplicities(aBSpline->NbKnots());
      for (int j = 1; j <= aBSpline->NbKnots(); ++j)
      {
        aKnotsBuilder.set(j - 1, aBSpline->Knot(j));
        aMultsBuilder.set(j - 1, aBSpline->Multiplicity(j));
      }

      auto aPolesBuilder = aBuilder.initPoles(aBSpline->NbPoles());
      for (int j = 1; j <= aBSpline->NbPoles(); ++j)
      {
        SetVec2(aPolesBuilder[j - 1], aBSpline->Pole(j));
      }

      if (aBSpline->IsRational())
      {
        auto aWeightsBuilder = aBuilder.initWeights(aBSpline->NbPoles());
        for (int j = 1; j <= aBSpline->NbPoles(); ++j)
        {
          aWeightsBuilder.set(j - 1, aBSpline->Weight(j));
        }
      }
    }
    else if (Handle(Geom2d_TrimmedCurve) aTrimmed = Handle(Geom2d_TrimmedCurve)::DownCast(aCurve))
    {
      auto aBuilder = aCurveBuilder.initTrimmedCurve();
      // TODO: Set basis curve reference
      aBuilder.setFirstParameter(aTrimmed->FirstParameter());
      aBuilder.setLastParameter(aTrimmed->LastParameter());
    }
    else if (Handle(Geom2d_OffsetCurve) anOffset = Handle(Geom2d_OffsetCurve)::DownCast(aCurve))
    {
      auto aBuilder = aCurveBuilder.initOffsetCurve();
      // TODO: Set basis curve reference
      aBuilder.setOffset(anOffset->Offset());
    }
  }

  // Write to file using POSIX file descriptor
  int fd = open(thePath.ToCString(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0)
  {
    return ODE_Status_WriteError;
  }

  capnp::writeMessageToFd(fd, aMessage);
  close(fd);
  return ODE_Status_OK;
}

//=================================================================================================

ODE_Status ODE_Curve2dFile::ReadFromFile(const TCollection_AsciiString& thePath)
{
  // Clear existing data
  Clear();

  // Open file using POSIX file descriptor
  int fd = open(thePath.ToCString(), O_RDONLY);
  if (fd < 0)
  {
    return ODE_Status_FileNotFound;
  }

  try
  {
    capnp::ReaderOptions options;
    options.traversalLimitInWords = 1024 * 1024 * 1024; // 1GB limit
    capnp::StreamFdMessageReader aMessageReader(fd, options);
    auto aFileReader = aMessageReader.getRoot<Curve2dFile>();
    auto aCurvesReader = aFileReader.getCurves();

    for (auto aCurveReader : aCurvesReader)
    {
      Handle(Geom2d_Curve) aCurve;

      // Dispatch by curve type
      switch (aCurveReader.which())
      {
        case Curve2d::LINE:
        {
          auto aLine = aCurveReader.getLine();
          gp_Pnt2d aLoc = GetPnt2d(aLine.getLocation());
          gp_Dir2d aDir = GetDir2d(aLine.getDirection());
          aCurve = new Geom2d_Line(aLoc, aDir);
          break;
        }

        case Curve2d::CIRCLE:
        {
          auto aCircle = aCurveReader.getCircle();
          gp_Ax22d aPos = GetAxis2d(aCircle.getPosition());
          Standard_Real aRadius = aCircle.getRadius();
          aCurve = new Geom2d_Circle(aPos, aRadius);
          break;
        }

        case Curve2d::ELLIPSE:
        {
          auto anEllipse = aCurveReader.getEllipse();
          gp_Ax22d aPos = GetAxis2d(anEllipse.getPosition());
          Standard_Real aMajorR = anEllipse.getMajorRadius();
          Standard_Real aMinorR = anEllipse.getMinorRadius();
          aCurve = new Geom2d_Ellipse(aPos, aMajorR, aMinorR);
          break;
        }

        case Curve2d::HYPERBOLA:
        {
          auto aHyperbola = aCurveReader.getHyperbola();
          gp_Ax22d aPos = GetAxis2d(aHyperbola.getPosition());
          Standard_Real aMajorR = aHyperbola.getMajorRadius();
          Standard_Real aMinorR = aHyperbola.getMinorRadius();
          aCurve = new Geom2d_Hyperbola(aPos, aMajorR, aMinorR);
          break;
        }

        case Curve2d::PARABOLA:
        {
          auto aParabola = aCurveReader.getParabola();
          gp_Ax22d aPos = GetAxis2d(aParabola.getPosition());
          Standard_Real aFocal = aParabola.getFocalLength();
          aCurve = new Geom2d_Parabola(aPos, aFocal);
          break;
        }

        case Curve2d::BEZIER_CURVE:
        {
          auto aBezier = aCurveReader.getBezierCurve();
          auto aPolesReader = aBezier.getPoles();

          TColgp_Array1OfPnt2d aPoles(1, aPolesReader.size());
          for (int j = 0; j < aPolesReader.size(); ++j)
          {
            aPoles.SetValue(j + 1, GetPnt2d(aPolesReader[j]));
          }

          if (aBezier.getRational())
          {
            auto aWeightsReader = aBezier.getWeights();
            TColStd_Array1OfReal aWeights(1, aWeightsReader.size());
            for (int j = 0; j < aWeightsReader.size(); ++j)
            {
              aWeights.SetValue(j + 1, aWeightsReader[j]);
            }
            aCurve = new Geom2d_BezierCurve(aPoles, aWeights);
          }
          else
          {
            aCurve = new Geom2d_BezierCurve(aPoles);
          }
          break;
        }

        case Curve2d::BSPLINE_CURVE:
        {
          auto aBSpline = aCurveReader.getBsplineCurve();
          auto aKnotsReader = aBSpline.getKnots();
          auto aMultsReader = aBSpline.getMultiplicities();
          auto aPolesReader = aBSpline.getPoles();

          TColStd_Array1OfReal aKnots(1, aKnotsReader.size());
          TColStd_Array1OfInteger aMults(1, aMultsReader.size());
          TColgp_Array1OfPnt2d aPoles(1, aPolesReader.size());

          for (int j = 0; j < aKnotsReader.size(); ++j)
          {
            aKnots.SetValue(j + 1, aKnotsReader[j]);
            aMults.SetValue(j + 1, aMultsReader[j]);
          }

          for (int j = 0; j < aPolesReader.size(); ++j)
          {
            aPoles.SetValue(j + 1, GetPnt2d(aPolesReader[j]));
          }

          if (aBSpline.getRational())
          {
            auto aWeightsReader = aBSpline.getWeights();
            TColStd_Array1OfReal aWeights(1, aWeightsReader.size());
            for (int j = 0; j < aWeightsReader.size(); ++j)
            {
              aWeights.SetValue(j + 1, aWeightsReader[j]);
            }
            aCurve = new Geom2d_BSplineCurve(aPoles, aWeights, aKnots, aMults, aBSpline.getDegree(), aBSpline.getPeriodic());
          }
          else
          {
            aCurve = new Geom2d_BSplineCurve(aPoles, aKnots, aMults, aBSpline.getDegree(), aBSpline.getPeriodic());
          }
          break;
        }

        case Curve2d::TRIMMED_CURVE:
        {
          auto aTrimmed = aCurveReader.getTrimmedCurve();
          // TODO: Resolve basis curve reference
          // For now, skip trimmed curves
          continue;
        }

        case Curve2d::OFFSET_CURVE:
        {
          auto anOffset = aCurveReader.getOffsetCurve();
          // TODO: Resolve basis curve reference
          // For now, skip offset curves
          continue;
        }
      }

      if (!aCurve.IsNull())
      {
        // Directly add to storage without deduplication
        CurveEntry aNewEntry;
        aNewEntry.MyCurve = aCurve;
        aNewEntry.SubIndexCount = 1;
        myCurves.Append(aNewEntry);
        myInstanceCount++;
      }
    }

    close(fd);
    return ODE_Status_OK;
  }
  catch (...)
  {
    close(fd);
    return ODE_Status_ReadError;
  }
}
