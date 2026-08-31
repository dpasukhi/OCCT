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

#ifndef _NCollection_PagedDataMap_HeaderFile
#define _NCollection_PagedDataMap_HeaderFile

#include <NCollection_PagedArray.hxx>

#include <NCollection_DefaultHasher.hxx>
#include <Standard_ProgramError.hxx>

#include <cstdint>
#include <utility>

//! Page-granular copy-on-write hash index.
//!
//! The bucket table uses open addressing. A copied map shares bucket pages;
//! insertion, replacement, and removal clone only pages containing modified
//! buckets. Capacity growth performs an explicit rehash and is amortized.
//! Retained copies are safe for concurrent const access and independently
//! synchronized updates. Concurrent mutation of the same map object requires
//! external synchronization.
template <typename TheKey,
          typename TheValue,
          typename TheHasher = NCollection_DefaultHasher<TheKey>>
class NCollection_PagedDataMap
{
public:
  class Iterator
  {
  public:
    explicit Iterator(const NCollection_PagedDataMap& theMap)
        : myMap(&theMap)
    {
      skipEmpty();
    }

    [[nodiscard]] bool More() const noexcept { return myIndex < myMap->myBuckets.Size(); }

    void Next()
    {
      ++myIndex;
      skipEmpty();
    }

    [[nodiscard]] const TheKey& Key() const { return myMap->myBuckets.Value(myIndex).Key; }

    [[nodiscard]] const TheValue& Value() const { return myMap->myBuckets.Value(myIndex).Value; }

  private:
    void skipEmpty()
    {
      while (myIndex < myMap->myBuckets.Size()
             && myMap->myBuckets.Value(myIndex).State != BucketState::Occupied)
      {
        ++myIndex;
      }
    }

    const NCollection_PagedDataMap* myMap   = nullptr;
    size_t                          myIndex = 0;
  };

  explicit NCollection_PagedDataMap(const int theInitialCapacity = 1)
  {
    initialize(capacityFor(static_cast<size_t>(theInitialCapacity < 1 ? 1 : theInitialCapacity)));
  }

  [[nodiscard]] size_t Size() const noexcept { return mySize; }

  [[nodiscard]] bool IsEmpty() const noexcept { return mySize == 0; }

  [[nodiscard]] const TheValue* Seek(const TheKey& theKey) const
  {
    const size_t anIndex = findIndex(theKey);
    return anIndex == THE_NOT_FOUND ? nullptr : &myBuckets.Value(anIndex).Value;
  }

  [[nodiscard]] TheValue* ChangeSeek(const TheKey& theKey)
  {
    const size_t anIndex = findIndex(theKey);
    return anIndex == THE_NOT_FOUND ? nullptr : &myBuckets.ChangeValue(anIndex).Value;
  }

  [[nodiscard]] bool IsBound(const TheKey& theKey) const
  {
    return findIndex(theKey) != THE_NOT_FOUND;
  }

  void Bind(const TheKey& theKey, const TheValue& theValue)
  {
    Standard_ProgramError_Raise_if(IsBound(theKey),
                                   "NCollection_PagedDataMap::Bind: key already bound");
    ensureInsertCapacity();
    bindWithoutResize(theKey, theValue);
  }

  void Bind(const TheKey& theKey, TheValue&& theValue)
  {
    Standard_ProgramError_Raise_if(IsBound(theKey),
                                   "NCollection_PagedDataMap::Bind: key already bound");
    ensureInsertCapacity();
    bindWithoutResize(theKey, std::move(theValue));
  }

  TheValue& TryBound(const TheKey& theKey, const TheValue& theValue)
  {
    const size_t anExisting = findIndex(theKey);
    if (anExisting != THE_NOT_FOUND)
    {
      return myBuckets.ChangeValue(anExisting).Value;
    }
    ensureInsertCapacity();
    return bindWithoutResize(theKey, theValue);
  }

  bool UnBind(const TheKey& theKey)
  {
    const size_t anIndex = findIndex(theKey);
    if (anIndex == THE_NOT_FOUND)
    {
      return false;
    }
    Bucket& aBucket = myBuckets.ChangeValue(anIndex);
    aBucket.State   = BucketState::Removed;
    aBucket.Key     = TheKey();
    aBucket.Value   = TheValue();
    --mySize;
    ++myRemoved;
    return true;
  }

  void Reserve(const size_t theCapacity)
  {
    const size_t aRequired = capacityFor((theCapacity * 10u + 6u) / 7u);
    if (aRequired > myBuckets.Size())
    {
      rehash(aRequired);
    }
  }

  void Clear(const bool = false) { initialize(THE_MIN_CAPACITY); }

private:
  enum class BucketState : uint8_t
  {
    Empty,
    Occupied,
    Removed
  };

  struct Bucket
  {
    TheKey      Key;
    TheValue    Value;
    BucketState State = BucketState::Empty;
  };

  static constexpr size_t THE_MIN_CAPACITY = 8;
  static constexpr size_t THE_PAGE_SIZE    = 64;
  static constexpr size_t THE_NOT_FOUND    = static_cast<size_t>(-1);

  static size_t capacityFor(size_t theCapacity)
  {
    size_t aCapacity = THE_MIN_CAPACITY;
    while (aCapacity < theCapacity)
    {
      aCapacity <<= 1;
    }
    return aCapacity;
  }

  void initialize(const size_t theCapacity)
  {
    myBuckets = NCollection_PagedArray<Bucket>(THE_PAGE_SIZE);
    myBuckets.Resize(theCapacity);
    mySize    = 0;
    myRemoved = 0;
  }

  [[nodiscard]] size_t findIndex(const TheKey& theKey) const
  {
    const size_t aMask   = myBuckets.Size() - 1;
    size_t       anIndex = myHasher(theKey) & aMask;
    for (size_t aProbe = 0; aProbe < myBuckets.Size(); ++aProbe)
    {
      const Bucket& aBucket = myBuckets.Value(anIndex);
      if (aBucket.State == BucketState::Empty)
      {
        return THE_NOT_FOUND;
      }
      if (aBucket.State == BucketState::Occupied && myHasher(aBucket.Key, theKey))
      {
        return anIndex;
      }
      anIndex = (anIndex + 1) & aMask;
    }
    return THE_NOT_FOUND;
  }

  void ensureInsertCapacity()
  {
    if ((mySize + myRemoved + 1) * 10u >= myBuckets.Size() * 7u)
    {
      rehash(mySize * 10u >= myBuckets.Size() * 6u ? myBuckets.Size() * 2u : myBuckets.Size());
    }
  }

  template <typename TheStoredValue>
  TheValue& bindWithoutResize(const TheKey& theKey, TheStoredValue&& theValue)
  {
    const size_t aMask         = myBuckets.Size() - 1;
    size_t       anIndex       = myHasher(theKey) & aMask;
    size_t       aRemovedIndex = THE_NOT_FOUND;
    for (;;)
    {
      const Bucket& aReadBucket = myBuckets.Value(anIndex);
      if (aReadBucket.State == BucketState::Removed && aRemovedIndex == THE_NOT_FOUND)
      {
        aRemovedIndex = anIndex;
      }
      else if (aReadBucket.State == BucketState::Empty)
      {
        const size_t aTarget = aRemovedIndex == THE_NOT_FOUND ? anIndex : aRemovedIndex;
        Bucket&      aBucket = myBuckets.ChangeValue(aTarget);
        aBucket.Key          = theKey;
        aBucket.Value        = std::forward<TheStoredValue>(theValue);
        aBucket.State        = BucketState::Occupied;
        ++mySize;
        if (aRemovedIndex != THE_NOT_FOUND)
        {
          --myRemoved;
        }
        return aBucket.Value;
      }
      anIndex = (anIndex + 1) & aMask;
    }
  }

  void rehash(const size_t theCapacity)
  {
    NCollection_PagedArray<Bucket> anOldBuckets = std::move(myBuckets);
    initialize(capacityFor(theCapacity));
    for (size_t anIndex = 0; anIndex < anOldBuckets.Size(); ++anIndex)
    {
      const Bucket& aBucket = anOldBuckets.Value(anIndex);
      if (aBucket.State == BucketState::Occupied)
      {
        bindWithoutResize(aBucket.Key, aBucket.Value);
      }
    }
  }

  NCollection_PagedArray<Bucket> myBuckets{THE_PAGE_SIZE};
  size_t                         mySize    = 0;
  size_t                         myRemoved = 0;
  TheHasher                      myHasher;
};

#endif // _NCollection_PagedDataMap_HeaderFile
