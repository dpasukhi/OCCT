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

#include <ExtremaPC.hxx>

#include <limits>

bool ExtremaPC::IsValidTolerance(const double theTolerance)
{
  return std::isfinite(theTolerance) && theTolerance > 0.0;
}

//==================================================================================================

bool ExtremaPC::IsFinitePoint(const gp_Pnt& thePoint)
{
  return std::isfinite(thePoint.X()) && !Precision::IsInfinite(thePoint.X())
         && std::isfinite(thePoint.Y()) && !Precision::IsInfinite(thePoint.Y())
         && std::isfinite(thePoint.Z()) && !Precision::IsInfinite(thePoint.Z());
}

//==================================================================================================

bool ExtremaPC::IsClosedPeriodicDomain(const Domain1D& theDomain,
                                       const double    thePeriod,
                                       const double    theTolerance)
{
  if (!theDomain.IsValid() || !std::isfinite(thePeriod) || thePeriod <= 0.0
      || !std::isfinite(theTolerance) || theTolerance < 0.0)
  {
    return false;
  }

  const double aNbPeriods = std::round(theDomain.Length() / thePeriod);
  return aNbPeriods >= 1.0 && std::abs(theDomain.Length() - aNbPeriods * thePeriod) <= theTolerance;
}

//==================================================================================================

bool ExtremaPC::IsFiniteParameter(const double theParameter)
{
  return std::isfinite(theParameter) && !Precision::IsInfinite(theParameter);
}

//==================================================================================================

double ExtremaPC::Result::MinSquareDistance() const
{
  return Extrema.IsEmpty() ? std::numeric_limits<double>::infinity()
                           : Extrema.Value(MinIndex()).SquareDistance;
}

//==================================================================================================

size_t ExtremaPC::Result::MinIndex() const
{
  if (Extrema.IsEmpty())
  {
    return INVALID_INDEX;
  }

  size_t aMinIndex = 0;
  for (size_t anIndex = 1; anIndex < Extrema.Size(); ++anIndex)
  {
    if (Extrema.Value(anIndex).SquareDistance < Extrema.Value(aMinIndex).SquareDistance)
    {
      aMinIndex = anIndex;
    }
  }
  return aMinIndex;
}

//==================================================================================================

double ExtremaPC::Result::MaxSquareDistance() const
{
  return Extrema.IsEmpty() ? 0.0 : Extrema.Value(MaxIndex()).SquareDistance;
}

//==================================================================================================

size_t ExtremaPC::Result::MaxIndex() const
{
  if (Extrema.IsEmpty())
  {
    return INVALID_INDEX;
  }

  size_t aMaxIndex = 0;
  for (size_t anIndex = 1; anIndex < Extrema.Size(); ++anIndex)
  {
    if (Extrema.Value(anIndex).SquareDistance > Extrema.Value(aMaxIndex).SquareDistance)
    {
      aMaxIndex = anIndex;
    }
  }
  return aMaxIndex;
}

//==================================================================================================

void ExtremaPC::Result::Clear()
{
  Status = Status::NotDone;
  Extrema.Clear();
  InfiniteSquareDistance = 0.0;
}
