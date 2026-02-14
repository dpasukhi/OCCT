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

#include <gtest/gtest.h>

#include <Geom_BSplineSurface.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <NCollection_Array1.hxx>
#include <NCollection_Array2.hxx>
#include <Precision.hxx>
#include <gp_Pnt.hxx>

//==================================================================================================

namespace
{

occ::handle<Geom_BSplineSurface> createMultiSpanSurface()
{
  NCollection_Array2<gp_Pnt> aPoles(1, 4, 1, 4);
  for (int aUIndex = 1; aUIndex <= 4; ++aUIndex)
  {
    for (int aVIndex = 1; aVIndex <= 4; ++aVIndex)
    {
      const double aZ          = 0.1 * double((aUIndex - 2) * (aVIndex - 2));
      aPoles(aUIndex, aVIndex) = gp_Pnt(double(aUIndex - 1), double(aVIndex - 1), aZ);
    }
  }

  NCollection_Array1<double> aUKnots(1, 3);
  aUKnots(1) = 0.0;
  aUKnots(2) = 1.0;
  aUKnots(3) = 2.0;

  NCollection_Array1<double> aVKnots(1, 3);
  aVKnots(1) = 0.0;
  aVKnots(2) = 1.0;
  aVKnots(3) = 2.0;

  NCollection_Array1<int> aUMults(1, 3);
  aUMults(1) = 3;
  aUMults(2) = 1;
  aUMults(3) = 3;

  NCollection_Array1<int> aVMults(1, 3);
  aVMults(1) = 3;
  aVMults(2) = 1;
  aVMults(3) = 3;

  return new Geom_BSplineSurface(aPoles, aUKnots, aVKnots, aUMults, aVMults, 2, 2);
}

} // namespace

//==================================================================================================

TEST(GeomAdaptor_Surface_EvalPredictionTest, BSplineSurface_SameSpanPrediction_MatchesBaseline)
{
  const occ::handle<Geom_BSplineSurface> aSurface = createMultiSpanSurface();
  GeomAdaptor_Surface                    anAdaptor(aSurface);

  const double aU  = 0.35;
  const double aV  = 0.40;
  const double aU2 = 0.85; // same U span [0, 1]
  const double aV2 = 0.75; // same V span [0, 1]

  const std::optional<gp_Pnt> aD0Base = anAdaptor.EvalD0(aU, aV);
  const std::optional<gp_Pnt> aD0Pred = anAdaptor.EvalD0(aU, aV, aU2, aV2);
  ASSERT_TRUE(aD0Base.has_value());
  ASSERT_TRUE(aD0Pred.has_value());
  EXPECT_NEAR(aD0Base->Distance(*aD0Pred), 0.0, Precision::Confusion());

  const std::optional<Geom_Surface::ResD1> aD1Base = anAdaptor.EvalD1(aU, aV);
  const std::optional<Geom_Surface::ResD1> aD1Pred = anAdaptor.EvalD1(aU, aV, aU2, aV2);
  ASSERT_TRUE(aD1Base.has_value());
  ASSERT_TRUE(aD1Pred.has_value());
  EXPECT_NEAR(aD1Base->Point.Distance(aD1Pred->Point), 0.0, Precision::Confusion());
  EXPECT_NEAR((aD1Base->D1U - aD1Pred->D1U).Magnitude(), 0.0, Precision::Confusion());
  EXPECT_NEAR((aD1Base->D1V - aD1Pred->D1V).Magnitude(), 0.0, Precision::Confusion());

  const std::optional<Geom_Surface::ResD2> aD2Base = anAdaptor.EvalD2(aU, aV);
  const std::optional<Geom_Surface::ResD2> aD2Pred = anAdaptor.EvalD2(aU, aV, aU2, aV2);
  ASSERT_TRUE(aD2Base.has_value());
  ASSERT_TRUE(aD2Pred.has_value());
  EXPECT_NEAR(aD2Base->Point.Distance(aD2Pred->Point), 0.0, Precision::Confusion());
  EXPECT_NEAR((aD2Base->D1U - aD2Pred->D1U).Magnitude(), 0.0, Precision::Confusion());
  EXPECT_NEAR((aD2Base->D1V - aD2Pred->D1V).Magnitude(), 0.0, Precision::Confusion());
  EXPECT_NEAR((aD2Base->D2U - aD2Pred->D2U).Magnitude(), 0.0, Precision::Confusion());
  EXPECT_NEAR((aD2Base->D2V - aD2Pred->D2V).Magnitude(), 0.0, Precision::Confusion());
  EXPECT_NEAR((aD2Base->D2UV - aD2Pred->D2UV).Magnitude(), 0.0, Precision::Confusion());
}

//==================================================================================================

TEST(GeomAdaptor_Surface_EvalPredictionTest, BSplineSurface_DifferentSpanPrediction_MatchesBaseline)
{
  const occ::handle<Geom_BSplineSurface> aSurface = createMultiSpanSurface();
  GeomAdaptor_Surface                    anAdaptor(aSurface);

  const double aU  = 0.35;
  const double aV  = 0.40;
  const double aU2 = 1.35; // different U span
  const double aV2 = 1.25; // different V span

  const std::optional<gp_Pnt> aD0Base = anAdaptor.EvalD0(aU, aV);
  const std::optional<gp_Pnt> aD0Pred = anAdaptor.EvalD0(aU, aV, aU2, aV2);
  ASSERT_TRUE(aD0Base.has_value());
  ASSERT_TRUE(aD0Pred.has_value());
  EXPECT_NEAR(aD0Base->Distance(*aD0Pred), 0.0, Precision::Confusion());

  const std::optional<Geom_Surface::ResD1> aD1Base = anAdaptor.EvalD1(aU, aV);
  const std::optional<Geom_Surface::ResD1> aD1Pred = anAdaptor.EvalD1(aU, aV, aU2, aV2);
  ASSERT_TRUE(aD1Base.has_value());
  ASSERT_TRUE(aD1Pred.has_value());
  EXPECT_NEAR(aD1Base->Point.Distance(aD1Pred->Point), 0.0, Precision::Confusion());
  EXPECT_NEAR((aD1Base->D1U - aD1Pred->D1U).Magnitude(), 0.0, Precision::Confusion());
  EXPECT_NEAR((aD1Base->D1V - aD1Pred->D1V).Magnitude(), 0.0, Precision::Confusion());

  const std::optional<Geom_Surface::ResD2> aD2Base = anAdaptor.EvalD2(aU, aV);
  const std::optional<Geom_Surface::ResD2> aD2Pred = anAdaptor.EvalD2(aU, aV, aU2, aV2);
  ASSERT_TRUE(aD2Base.has_value());
  ASSERT_TRUE(aD2Pred.has_value());
  EXPECT_NEAR(aD2Base->Point.Distance(aD2Pred->Point), 0.0, Precision::Confusion());
  EXPECT_NEAR((aD2Base->D1U - aD2Pred->D1U).Magnitude(), 0.0, Precision::Confusion());
  EXPECT_NEAR((aD2Base->D1V - aD2Pred->D1V).Magnitude(), 0.0, Precision::Confusion());
  EXPECT_NEAR((aD2Base->D2U - aD2Pred->D2U).Magnitude(), 0.0, Precision::Confusion());
  EXPECT_NEAR((aD2Base->D2V - aD2Pred->D2V).Magnitude(), 0.0, Precision::Confusion());
  EXPECT_NEAR((aD2Base->D2UV - aD2Pred->D2UV).Magnitude(), 0.0, Precision::Confusion());
}
