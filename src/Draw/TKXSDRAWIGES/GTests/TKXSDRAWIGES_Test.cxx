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

#include <gtest/gtest.h>

// Include necessary OCCT headers
#include <Standard_Version.hxx>

// Test fixture for TKXSDRAWIGES tests
class TKXSDRAWIGESTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code that will be run before each test
  }

  void TearDown() override {
    // Cleanup code that will be run after each test
  }
};

// Sample test checking OCCT version is defined
TEST_F(TKXSDRAWIGESTest, VersionDefined) {
  // Simple test that always passes
  EXPECT_GT(OCC_VERSION_MAJOR, 0);
  EXPECT_GT(OCC_VERSION_MINOR, 0);
}
