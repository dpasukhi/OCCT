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

#include <NCollection_BitDynamicArray.hxx>
#include <NCollection_PagedArray.hxx>
#include <NCollection_PagedDataMap.hxx>

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

namespace
{
struct PagedTestValue
{
  std::shared_ptr<int> Payload;
};
} // namespace

TEST(NCollection_PagedArrayTest, CopyOnWriteDetachesOnlyModifiedSnapshot)
{
  NCollection_PagedArray<int> aBase(4);
  for (int anIndex = 0; anIndex < 12; ++anIndex)
  {
    aBase.Append(anIndex);
  }

  NCollection_PagedArray<int> aChanged = aBase;
  aChanged.ChangeValue(5)              = 50;
  aChanged.Resize(14, 7);

  EXPECT_EQ(aBase.Size(), 12u);
  EXPECT_EQ(aBase.Value(5), 5);
  EXPECT_EQ(aChanged.Value(5), 50);
  EXPECT_EQ(aChanged.Value(12), 7);
  EXPECT_EQ(aChanged.Value(13), 7);
}

TEST(NCollection_PagedArrayTest, SharedSnapshotSupportsConcurrentReadsAndIndependentUpdates)
{
  NCollection_PagedArray<int> aValues(8);
  for (int anIndex = 0; anIndex < 128; ++anIndex)
  {
    aValues.Append(anIndex);
  }
  const NCollection_PagedArray<int> aBase = aValues;

  std::atomic<bool>        isValid{true};
  std::vector<std::thread> aThreads;
  for (int aThread = 0; aThread < 8; ++aThread)
  {
    aThreads.emplace_back([&aBase, &isValid, aThread]() {
      NCollection_PagedArray<int> aChanged               = aBase;
      aChanged.ChangeValue(static_cast<size_t>(aThread)) = -aThread - 1;
      for (int anIndex = 0; anIndex < 128; ++anIndex)
      {
        if (aBase.Value(static_cast<size_t>(anIndex)) != anIndex)
        {
          isValid.store(false, std::memory_order_relaxed);
        }
      }
      if (aChanged.Value(static_cast<size_t>(aThread)) != -aThread - 1)
      {
        isValid.store(false, std::memory_order_relaxed);
      }
    });
  }
  for (std::thread& aThread : aThreads)
  {
    aThread.join();
  }
  EXPECT_TRUE(isValid.load(std::memory_order_relaxed));
}

TEST(NCollection_PagedArrayTest, ShrinkAndExtensionRestoreDefaultValues)
{
  NCollection_PagedArray<int> aValues(4);
  aValues.Resize(4, 9);
  aValues.Resize(1);

  EXPECT_EQ(aValues.Appended(), 0);

  aValues.Resize(1);
  aValues.SetValue(3, 7);
  EXPECT_EQ(aValues.Value(1), 0);
  EXPECT_EQ(aValues.Value(2), 0);
  EXPECT_EQ(aValues.Value(3), 7);
}

TEST(NCollection_PagedArrayTest, ShrinkReleasesRetainedPageValues)
{
  NCollection_PagedArray<PagedTestValue> aValues(4);
  aValues.Resize(3);
  aValues.ChangeValue(2).Payload = std::make_shared<int>(42);
  std::weak_ptr<int> aPayload    = aValues.Value(2).Payload;

  aValues.Resize(1);
  EXPECT_TRUE(aPayload.expired());

  aValues.Resize(3);
  EXPECT_FALSE(aValues.Value(1).Payload);
  EXPECT_FALSE(aValues.Value(2).Payload);
}

TEST(NCollection_PagedArrayTest, ShrinkDetachesSharedTailPage)
{
  NCollection_PagedArray<PagedTestValue> aBase(4);
  aBase.Resize(3);
  aBase.ChangeValue(2).Payload = std::make_shared<int>(42);

  NCollection_PagedArray<PagedTestValue> aChanged = aBase;
  aChanged.Resize(1);
  aChanged.Resize(3);

  ASSERT_TRUE(aBase.Value(2).Payload);
  EXPECT_EQ(*aBase.Value(2).Payload, 42);
  EXPECT_FALSE(aChanged.Value(2).Payload);
}

TEST(NCollection_PagedArrayTest, SparseSetInitializesEveryGap)
{
  NCollection_PagedArray<int> aValues(3);
  aValues.SetValue(10, 42);

  ASSERT_EQ(aValues.Size(), 11u);
  for (size_t anIndex = 0; anIndex < 10; ++anIndex)
  {
    EXPECT_EQ(aValues.Value(anIndex), 0) << "index " << anIndex;
  }
  EXPECT_EQ(aValues.Value(10), 42);
}

TEST(NCollection_BitDynamicArrayTest, TailMaskAndCopyOnWriteAreStable)
{
  NCollection_BitDynamicArray aBase;
  aBase.Resize(70);
  aBase.SetAll();
  EXPECT_EQ(aBase.Block(0), ~uint64_t(0));
  EXPECT_EQ(aBase.Block(1), uint64_t(0x3f));

  NCollection_BitDynamicArray aChanged = aBase;
  aChanged.Clear(65);
  aChanged.Resize(66);

  EXPECT_TRUE(aBase.Test(65));
  EXPECT_FALSE(aChanged.Test(65));
  EXPECT_EQ(aChanged.Block(1), uint64_t(0x1));
}

TEST(NCollection_PagedDataMapTest, SnapshotsKeepIndependentBucketsAndValues)
{
  NCollection_PagedDataMap<int, int> aBase;
  for (int aKey = 0; aKey < 100; ++aKey)
  {
    aBase.Bind(aKey, aKey * 2);
  }

  NCollection_PagedDataMap<int, int> aChanged = aBase;
  ASSERT_NE(aChanged.ChangeSeek(42), nullptr);
  *aChanged.ChangeSeek(42) = 7;
  EXPECT_TRUE(aChanged.UnBind(10));
  aChanged.Bind(200, 400);

  ASSERT_NE(aBase.Seek(42), nullptr);
  EXPECT_EQ(*aBase.Seek(42), 84);
  EXPECT_TRUE(aBase.IsBound(10));
  EXPECT_FALSE(aBase.IsBound(200));
  EXPECT_EQ(*aChanged.Seek(42), 7);
}
