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

#include <CSLib.hxx>

#include <gtest/gtest.h>

#include <Standard_Real.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <gp.hxx>
#include <Precision.hxx>
#include <TColgp_Array2OfVec.hxx>

// Test basic normal computation from two tangent vectors
TEST(CSLibNormalTest, NormalFromPerpendicularTangents)
{
  // Test with perpendicular unit vectors in X and Y directions
  gp_Vec aD1U(1.0, 0.0, 0.0);  // Tangent in U direction
  gp_Vec aD1V(0.0, 1.0, 0.0);  // Tangent in V direction

  gp_Dir aNormal;
  CSLib_DerivativeStatus aStatus;
  Standard_Real aSinTol = 1.0e-6;

  CSLib::Normal(aD1U, aD1V, aSinTol, aStatus, aNormal);

  EXPECT_EQ(aStatus, CSLib_Done) << "Normal computation should succeed";

  // Expected normal: Z direction (cross product of X and Y)
  gp_Dir aExpectedNormal(0.0, 0.0, 1.0);

  EXPECT_NEAR(aNormal.X(), aExpectedNormal.X(), 1.0e-10) << "Normal X component";
  EXPECT_NEAR(aNormal.Y(), aExpectedNormal.Y(), 1.0e-10) << "Normal Y component";
  EXPECT_NEAR(aNormal.Z(), aExpectedNormal.Z(), 1.0e-10) << "Normal Z component";
}

// Test normal computation with non-unit tangent vectors
TEST(CSLibNormalTest, NormalFromScaledTangents)
{
  // Test with scaled tangent vectors
  gp_Vec aD1U(2.0, 0.0, 0.0);
  gp_Vec aD1V(0.0, 3.0, 0.0);

  gp_Dir aNormal;
  CSLib_DerivativeStatus aStatus;
  Standard_Real aSinTol = 1.0e-6;

  CSLib::Normal(aD1U, aD1V, aSinTol, aStatus, aNormal);

  EXPECT_EQ(aStatus, CSLib_Done) << "Should succeed with scaled tangents";

  // Normal should still point in Z direction
  gp_Dir aExpectedNormal(0.0, 0.0, 1.0);

  EXPECT_NEAR(aNormal.X(), aExpectedNormal.X(), 1.0e-10);
  EXPECT_NEAR(aNormal.Y(), aExpectedNormal.Y(), 1.0e-10);
  EXPECT_NEAR(aNormal.Z(), aExpectedNormal.Z(), 1.0e-10);
}

// Test null derivative detection
TEST(CSLibNormalTest, NullD1UDerivative)
{
  gp_Vec aD1U(0.0, 0.0, 0.0);  // Null U derivative
  gp_Vec aD1V(0.0, 1.0, 0.0);

  gp_Dir aNormal;
  CSLib_DerivativeStatus aStatus;
  Standard_Real aSinTol = 1.0e-6;

  CSLib::Normal(aD1U, aD1V, aSinTol, aStatus, aNormal);

  EXPECT_EQ(aStatus, CSLib_D1uIsNull) << "Should detect null D1U";
}

// Test null V derivative detection
TEST(CSLibNormalTest, NullD1VDerivative)
{
  gp_Vec aD1U(1.0, 0.0, 0.0);
  gp_Vec aD1V(0.0, 0.0, 0.0);  // Null V derivative

  gp_Dir aNormal;
  CSLib_DerivativeStatus aStatus;
  Standard_Real aSinTol = 1.0e-6;

  CSLib::Normal(aD1U, aD1V, aSinTol, aStatus, aNormal);

  EXPECT_EQ(aStatus, CSLib_D1vIsNull) << "Should detect null D1V";
}

// Test parallel derivatives detection
TEST(CSLibNormalTest, ParallelDerivatives)
{
  gp_Vec aD1U(1.0, 0.0, 0.0);
  gp_Vec aD1V(2.0, 0.0, 0.0);  // Parallel to D1U

  gp_Dir aNormal;
  CSLib_DerivativeStatus aStatus;
  Standard_Real aSinTol = 1.0e-6;

  CSLib::Normal(aD1U, aD1V, aSinTol, aStatus, aNormal);

  EXPECT_EQ(aStatus, CSLib_D1uIsParallelD1v) << "Should detect parallel derivatives";
}

// Test both derivatives null
TEST(CSLibNormalTest, BothDerivativesNull)
{
  gp_Vec aD1U(0.0, 0.0, 0.0);
  gp_Vec aD1V(0.0, 0.0, 0.0);

  gp_Dir aNormal;
  CSLib_DerivativeStatus aStatus;
  Standard_Real aSinTol = 1.0e-6;

  CSLib::Normal(aD1U, aD1V, aSinTol, aStatus, aNormal);

  EXPECT_EQ(aStatus, CSLib_D1IsNull) << "Should detect both derivatives null";
}

// Test normal computation with magnitude tolerance
TEST(CSLibNormalTest, NormalWithMagnitudeTolerance)
{
  gp_Vec aD1U(1.0, 0.0, 0.0);
  gp_Vec aD1V(0.0, 1.0, 0.0);

  gp_Dir aNormal;
  CSLib_NormalStatus aStatus;
  Standard_Real aMagTol = 1.0e-7;

  CSLib::Normal(aD1U, aD1V, aMagTol, aStatus, aNormal);

  EXPECT_EQ(aStatus, CSLib_Defined) << "Normal should be defined";

  gp_Dir aExpectedNormal(0.0, 0.0, 1.0);
  EXPECT_NEAR(aNormal.X(), aExpectedNormal.X(), 1.0e-10);
  EXPECT_NEAR(aNormal.Y(), aExpectedNormal.Y(), 1.0e-10);
  EXPECT_NEAR(aNormal.Z(), aExpectedNormal.Z(), 1.0e-10);
}

// Test singular point detection with magnitude tolerance
TEST(CSLibNormalTest, SingularPointDetection)
{
  // Very small tangent vectors
  gp_Vec aD1U(1.0e-12, 0.0, 0.0);
  gp_Vec aD1V(0.0, 1.0e-12, 0.0);

  gp_Dir aNormal;
  CSLib_NormalStatus aStatus;
  Standard_Real aMagTol = 1.0e-7;

  CSLib::Normal(aD1U, aD1V, aMagTol, aStatus, aNormal);

  EXPECT_EQ(aStatus, CSLib_Singular) << "Should detect singular point";
}

// Test normal from second derivatives at singular point
TEST(CSLibNormalTest, NormalFromSecondDerivatives)
{
  // At singular point: D1U and D1V are null
  gp_Vec aD1U(0.0, 0.0, 0.0);
  gp_Vec aD1V(0.0, 0.0, 0.0);

  // Second derivatives
  gp_Vec aD2U(1.0, 0.0, 0.0);
  gp_Vec aD2V(0.0, 1.0, 0.0);
  gp_Vec aD2UV(0.0, 0.0, 0.0);

  gp_Dir aNormal;
  Standard_Boolean isDone;
  CSLib_NormalStatus aStatus;
  Standard_Real aSinTol = 1.0e-6;

  CSLib::Normal(aD1U, aD1V, aD2U, aD2V, aD2UV, aSinTol, isDone, aStatus, aNormal);

  // Should attempt to compute normal from second derivatives
  // Result depends on the specific derivatives
  EXPECT_TRUE(aStatus == CSLib_D1NuIsNull || aStatus == CSLib_D1NvIsNull ||
              aStatus == CSLib_D1NuIsParallelD1Nv || aStatus == CSLib_InfinityOfSolutions ||
              aStatus == CSLib_D1NIsNull);
}

// Test optimized magnitude calculation (performance improvement)
TEST(CSLibNormalTest, OptimizedMagnitudeCalculation)
{
  // This test verifies the optimized code path using squared magnitudes
  gp_Vec aD1U(3.0, 4.0, 0.0);  // Magnitude = 5
  gp_Vec aD1V(0.0, 5.0, 12.0); // Magnitude = 13

  gp_Dir aNormal;
  CSLib_NormalStatus aStatus;
  Standard_Real aMagTol = 1.0e-7;

  CSLib::Normal(aD1U, aD1V, aMagTol, aStatus, aNormal);

  EXPECT_EQ(aStatus, CSLib_Defined) << "Should compute normal successfully";

  // Verify the normal is perpendicular to both tangents
  gp_Vec aNormalVec(aNormal.XYZ());
  Standard_Real aDot1 = std::abs(aNormalVec.Dot(aD1U));
  Standard_Real aDot2 = std::abs(aNormalVec.Dot(aD1V));

  EXPECT_LT(aDot1, 1.0e-10) << "Normal should be perpendicular to D1U";
  EXPECT_LT(aDot2, 1.0e-10) << "Normal should be perpendicular to D1V";
}

// Test DNNUV computation
TEST(CSLibNormalTest, DNNUVComputation)
{
  // Create a simple array of surface derivatives
  TColgp_Array2OfVec aDerSurf(0, 2, 0, 2);

  // Fill with simple derivative pattern for a plane
  aDerSurf(0, 0) = gp_Vec(0.0, 0.0, 0.0);  // Point
  aDerSurf(1, 0) = gp_Vec(1.0, 0.0, 0.0);  // D1U
  aDerSurf(0, 1) = gp_Vec(0.0, 1.0, 0.0);  // D1V
  aDerSurf(2, 0) = gp_Vec(0.0, 0.0, 0.0);  // D2U
  aDerSurf(0, 2) = gp_Vec(0.0, 0.0, 0.0);  // D2V
  aDerSurf(1, 1) = gp_Vec(0.0, 0.0, 0.0);  // D2UV
  aDerSurf(2, 1) = gp_Vec(0.0, 0.0, 0.0);
  aDerSurf(1, 2) = gp_Vec(0.0, 0.0, 0.0);
  aDerSurf(2, 2) = gp_Vec(0.0, 0.0, 0.0);

  // Compute derivative of non-normalized normal
  gp_Vec aDNNUV = CSLib::DNNUV(0, 0, aDerSurf);

  // For a plane with perpendicular U and V derivatives,
  // the non-normalized normal D1U x D1V should be (0, 0, 1)
  EXPECT_NEAR(aDNNUV.X(), 0.0, 1.0e-10);
  EXPECT_NEAR(aDNNUV.Y(), 0.0, 1.0e-10);
  EXPECT_NEAR(aDNNUV.Z(), 1.0, 1.0e-10);
}
