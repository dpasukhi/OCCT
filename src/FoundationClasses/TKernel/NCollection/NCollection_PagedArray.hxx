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

#ifndef _NCollection_PagedArray_HeaderFile
#define _NCollection_PagedArray_HeaderFile

#include <NCollection_Array1.hxx>

#include <algorithm>
#include <array>
#include <memory>
#include <utility>
#include <variant>

//! Paged array with page-granular copy-on-write ownership.
//!
//! Copying the array retains one radix-tree root. Const reads address shared
//! immutable pages directly; the first write clones the radix path and the
//! touched page. This gives snapshots O(1) retention, bounded indexed reads,
//! and mutation cost independent of the total number of pages.
//! Concurrent const access to retained copies is safe. Mutation requires an
//! externally serialized, privately owned array; atomic reference counts do
//! not make simultaneous writes to the same array safe.
template <typename TheItemType>
class NCollection_PagedArray
{
public:
  using value_type      = TheItemType;
  using reference       = TheItemType&;
  using const_reference = const TheItemType&;

  //! Construct an empty array.
  explicit NCollection_PagedArray(const size_t thePageSize = 256)
      : myRoot(std::make_shared<PageTableNode>(true)),
        myPageSize(thePageSize == 0 ? 1 : thePageSize)
  {
  }

  //! Return the number of visible values.
  [[nodiscard]] size_t Size() const noexcept { return mySize; }

  //! Return true when no values are visible.
  [[nodiscard]] bool IsEmpty() const noexcept { return mySize == 0; }

  //! Return a value without detaching its page.
  [[nodiscard]] const_reference Value(const size_t theIndex) const noexcept
  {
    return page(pageIndex(theIndex)).Values.At(pageOffset(theIndex));
  }

  [[nodiscard]] const_reference operator[](const size_t theIndex) const noexcept
  {
    return Value(theIndex);
  }

  //! Return mutable access, cloning a shared page before exposing it.
  [[nodiscard]] reference ChangeValue(const size_t theIndex)
  {
    return changePage(pageIndex(theIndex)).Values.ChangeAt(pageOffset(theIndex));
  }

  [[nodiscard]] reference operator[](const size_t theIndex) { return ChangeValue(theIndex); }

  //! Append a default-constructed value.
  reference Appended()
  {
    ensurePage(pageIndex(mySize));
    reference aValue = ChangeValue(mySize);
    aValue           = TheItemType();
    ++mySize;
    return aValue;
  }

  //! Append a copied value.
  reference Append(const TheItemType& theValue)
  {
    reference aValue = Appended();
    aValue           = theValue;
    return aValue;
  }

  //! Append a moved value.
  reference Append(TheItemType&& theValue)
  {
    reference aValue = Appended();
    aValue           = std::move(theValue);
    return aValue;
  }

  //! Set a value, extending the visible range with default values when needed.
  reference SetValue(const size_t theIndex, const TheItemType& theValue)
  {
    if (theIndex >= mySize)
    {
      Resize(theIndex + 1);
    }
    reference aValue = ChangeValue(theIndex);
    aValue           = theValue;
    return aValue;
  }

  //! Set a moved value, extending the visible range when needed.
  reference SetValue(const size_t theIndex, TheItemType&& theValue)
  {
    if (theIndex >= mySize)
    {
      Resize(theIndex + 1);
    }
    reference aValue = ChangeValue(theIndex);
    aValue           = std::move(theValue);
    return aValue;
  }

  //! Change the visible size.
  void Resize(const size_t theSize, const TheItemType& theValue = TheItemType())
  {
    if (theSize > mySize)
    {
      const size_t anOldSize = mySize;
      ensurePage(pageIndex(theSize - 1));
      mySize = theSize;
      for (size_t anIndex = anOldSize; anIndex < theSize; ++anIndex)
      {
        ChangeValue(anIndex) = theValue;
      }
      return;
    }
    if (theSize == mySize)
    {
      return;
    }
    if (theSize == 0)
    {
      Clear();
      return;
    }

    const size_t aPageCount = pageIndex(theSize - 1) + 1;
    const size_t anEnd      = std::min(mySize, aPageCount * myPageSize);
    for (size_t anIndex = theSize; anIndex < anEnd; ++anIndex)
    {
      ChangeValue(anIndex) = TheItemType();
    }
    for (size_t aPageIndex = aPageCount; aPageIndex < myPageCount; ++aPageIndex)
    {
      releasePage(aPageIndex);
    }
    mySize      = theSize;
    myPageCount = aPageCount;
    collapseRoot();
  }

  //! Clear the visible array and release this array's page references.
  void Clear()
  {
    myRoot      = std::make_shared<PageTableNode>(true);
    myDepth     = 0;
    myPageCount = 0;
    mySize      = 0;
  }

  //! Return the physical page size.
  [[nodiscard]] size_t PageSize() const noexcept { return myPageSize; }

  //! Return the number of retained pages.
  [[nodiscard]] size_t NbPages() const noexcept { return myPageCount; }

private:
  struct Page
  {
    explicit Page(const size_t theSize)
        : Values(theSize)
    {
    }

    NCollection_Array1<TheItemType> Values;
  };

  static constexpr size_t THE_BRANCH_BITS = 6;
  static constexpr size_t THE_BRANCH_SIZE = size_t(1) << THE_BRANCH_BITS;
  static constexpr size_t THE_BRANCH_MASK = THE_BRANCH_SIZE - 1;
  static constexpr size_t THE_MAX_DEPTH   = (sizeof(size_t) * 8 - 1) / THE_BRANCH_BITS;

  struct PageTableNode
  {
    using BranchSlots = std::array<std::shared_ptr<PageTableNode>, THE_BRANCH_SIZE>;
    using PageSlots   = std::array<std::shared_ptr<Page>, THE_BRANCH_SIZE>;
    using SlotStorage = std::variant<BranchSlots, PageSlots>;

    explicit PageTableNode(const bool theIsLeaf)
        : Slots(theIsLeaf ? SlotStorage(std::in_place_index<1>)
                          : SlotStorage(std::in_place_index<0>))
    {
    }

    [[nodiscard]] BranchSlots& ChangeBranches() { return std::get<BranchSlots>(Slots); }

    [[nodiscard]] const BranchSlots& Branches() const { return std::get<BranchSlots>(Slots); }

    [[nodiscard]] PageSlots& ChangePages() { return std::get<PageSlots>(Slots); }

    [[nodiscard]] const PageSlots& Pages() const { return std::get<PageSlots>(Slots); }

    [[nodiscard]] bool IsEmpty() const
    {
      if (Slots.index() == 0)
      {
        return std::all_of(
          Branches().begin(),
          Branches().end(),
          [](const std::shared_ptr<PageTableNode>& theChild) { return theChild == nullptr; });
      }
      return std::all_of(Pages().begin(), Pages().end(), [](const std::shared_ptr<Page>& thePage) {
        return thePage == nullptr;
      });
    }

    SlotStorage Slots;
  };

  [[nodiscard]] size_t pageIndex(const size_t theIndex) const noexcept
  {
    return theIndex / myPageSize;
  }

  [[nodiscard]] size_t pageOffset(const size_t theIndex) const noexcept
  {
    return theIndex % myPageSize;
  }

  void ensurePage(const size_t thePageIndex)
  {
    while (myDepth < THE_MAX_DEPTH && (thePageIndex >> ((myDepth + 1) * THE_BRANCH_BITS)) != 0)
    {
      std::shared_ptr<PageTableNode> aRoot = std::make_shared<PageTableNode>(false);
      aRoot->ChangeBranches()[0]           = myRoot;
      myRoot                               = std::move(aRoot);
      ++myDepth;
    }
    while (myPageCount <= thePageIndex)
    {
      (void)changePage(myPageCount);
      ++myPageCount;
    }
  }

  [[nodiscard]] const Page& page(const size_t thePageIndex) const noexcept
  {
    const PageTableNode* aNode = myRoot.get();
    for (size_t aLevel = myDepth; aLevel > 0; --aLevel)
    {
      const size_t aSlot = (thePageIndex >> (aLevel * THE_BRANCH_BITS)) & THE_BRANCH_MASK;
      aNode              = aNode->Branches()[aSlot].get();
    }
    return *aNode->Pages()[thePageIndex & THE_BRANCH_MASK];
  }

  Page& changePage(const size_t thePageIndex)
  {
    if (myRoot.use_count() != 1)
    {
      myRoot = std::make_shared<PageTableNode>(*myRoot);
    }

    PageTableNode* aNode = myRoot.get();
    for (size_t aLevel = myDepth; aLevel > 0; --aLevel)
    {
      const size_t aSlot = (thePageIndex >> (aLevel * THE_BRANCH_BITS)) & THE_BRANCH_MASK;
      std::shared_ptr<PageTableNode>& aChild = aNode->ChangeBranches()[aSlot];
      if (aChild == nullptr)
      {
        aChild = std::make_shared<PageTableNode>(aLevel == 1);
      }
      else if (aChild.use_count() != 1)
      {
        aChild = std::make_shared<PageTableNode>(*aChild);
      }
      aNode = aChild.get();
    }

    std::shared_ptr<Page>& aPage = aNode->ChangePages()[thePageIndex & THE_BRANCH_MASK];
    if (aPage == nullptr)
    {
      aPage = std::make_shared<Page>(myPageSize);
    }
    if (aPage.use_count() != 1)
    {
      aPage = std::make_shared<Page>(*aPage);
    }
    return *aPage;
  }

  void releasePage(const size_t thePageIndex)
  {
    if (myRoot.use_count() != 1)
    {
      myRoot = std::make_shared<PageTableNode>(*myRoot);
    }

    std::array<PageTableNode*, THE_MAX_DEPTH> aParents{};
    std::array<size_t, THE_MAX_DEPTH>         aSlots{};
    size_t                                    aPathLength = 0;
    PageTableNode*                            aNode       = myRoot.get();
    for (size_t aLevel = myDepth; aLevel > 0; --aLevel)
    {
      const size_t aSlot = (thePageIndex >> (aLevel * THE_BRANCH_BITS)) & THE_BRANCH_MASK;
      std::shared_ptr<PageTableNode>& aChild = aNode->ChangeBranches()[aSlot];
      if (aChild.use_count() != 1)
      {
        aChild = std::make_shared<PageTableNode>(*aChild);
      }
      aParents[aPathLength] = aNode;
      aSlots[aPathLength]   = aSlot;
      ++aPathLength;
      aNode = aChild.get();
    }
    aNode->ChangePages()[thePageIndex & THE_BRANCH_MASK].reset();

    while (aPathLength > 0 && aNode->IsEmpty())
    {
      --aPathLength;
      PageTableNode* aParent = aParents[aPathLength];
      aParent->ChangeBranches()[aSlots[aPathLength]].reset();
      aNode = aParent;
    }
  }

  void collapseRoot()
  {
    while (myDepth > 0)
    {
      const typename PageTableNode::BranchSlots& aBranches        = myRoot->Branches();
      bool                                       hasOtherBranches = false;
      for (size_t aSlot = 1; aSlot < THE_BRANCH_SIZE; ++aSlot)
      {
        if (aBranches[aSlot] != nullptr)
        {
          hasOtherBranches = true;
          break;
        }
      }
      if (hasOtherBranches || aBranches[0] == nullptr)
      {
        break;
      }
      std::shared_ptr<PageTableNode> aChild = aBranches[0];
      myRoot                                = std::move(aChild);
      --myDepth;
    }
  }

  std::shared_ptr<PageTableNode> myRoot;
  size_t                         myPageSize  = 256;
  size_t                         mySize      = 0;
  size_t                         myPageCount = 0;
  size_t                         myDepth     = 0;
};

#endif // _NCollection_PagedArray_HeaderFile
