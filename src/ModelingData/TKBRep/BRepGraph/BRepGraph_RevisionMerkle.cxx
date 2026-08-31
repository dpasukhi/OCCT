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

#include <BRepGraph_RevisionMerkle.hxx>

#include <NCollection_LinearVector.hxx>
#include <Standard_DomainError.hxx>

#include <algorithm>
#include <array>

namespace
{
constexpr int      THE_RADIX          = 256;
constexpr int      THE_KEY_BYTES      = 8;
constexpr uint32_t THE_FORMAT_VERSION = 1;
constexpr size_t   THE_MAX_INPUT_SIZE = 4 + 4 + 1 + 2 + THE_RADIX * (1 + 4 * 8);

struct HashInput
{
  void AppendByte(const uint8_t theValue) { Bytes[Size++] = theValue; }

  void AppendUInt16(const uint16_t theValue)
  {
    AppendByte(static_cast<uint8_t>(theValue));
    AppendByte(static_cast<uint8_t>(theValue >> 8));
  }

  void AppendUInt32(const uint32_t theValue)
  {
    for (int aByte = 0; aByte < 4; ++aByte)
    {
      AppendByte(static_cast<uint8_t>(theValue >> (aByte * 8)));
    }
  }

  void AppendUInt64(const uint64_t theValue)
  {
    for (int aByte = 0; aByte < 8; ++aByte)
    {
      AppendByte(static_cast<uint8_t>(theValue >> (aByte * 8)));
    }
  }

  void AppendHash(const BRepGraph_RevisionHash& theHash)
  {
    for (const uint64_t aWord : theHash.Words)
    {
      AppendUInt64(aWord);
    }
  }

  BRepGraph_RevisionHash Hash() const
  {
    return BRepGraph_RevisionHash::Hasher::Bytes(Bytes.data(), Size);
  }

  std::array<uint8_t, THE_MAX_INPUT_SIZE> Bytes;
  size_t                                  Size = 0;
};

uint8_t keySlot(const uint64_t theKey, const int theDepth)
{
  return static_cast<uint8_t>(theKey >> ((THE_KEY_BYTES - theDepth - 1) * 8));
}

BRepGraph_RevisionHash leafHash(const uint64_t theKey, const BRepGraph_RevisionHash& theValue)
{
  HashInput anInput;
  anInput.AppendUInt32(0x4d4c4546u); // MLEF
  anInput.AppendUInt32(THE_FORMAT_VERSION);
  anInput.AppendUInt64(theKey);
  anInput.AppendHash(theValue);
  return anInput.Hash();
}
} // namespace

struct BRepGraph_RevisionMerkle::Node
{
  struct Child
  {
    uint8_t Slot;
    NodePtr Node;
  };

  using Children = NCollection_LinearVector<Child>;

  static size_t ChildIndex(const Children& theChildren, const uint8_t theSlot)
  {
    const auto anIt = std::lower_bound(
      theChildren.begin(),
      theChildren.end(),
      theSlot,
      [](const Child& theChild, const uint8_t theValue) { return theChild.Slot < theValue; });
    return static_cast<size_t>(anIt - theChildren.begin());
  }

  Node(const uint64_t theKey, const BRepGraph_RevisionHash& theHash)
      : Hash(theHash),
        Key(theKey),
        Count(1),
        IsLeaf(true)
  {
  }

  Node(const Children& theChildren, const int theDepth)
      : ChildrenBySlot(theChildren),
        IsLeaf(false)
  {
    HashInput anInput;
    anInput.AppendUInt32(0x4d42524eu); // MBRN
    anInput.AppendUInt32(THE_FORMAT_VERSION);
    anInput.AppendByte(static_cast<uint8_t>(theDepth));
    anInput.AppendUInt16(static_cast<uint16_t>(ChildrenBySlot.Size()));
    for (const Child& aChild : ChildrenBySlot)
    {
      Count += aChild.Node->Count;
      anInput.AppendByte(aChild.Slot);
      anInput.AppendHash(aChild.Node->Hash);
    }
    Hash = anInput.Hash();
  }

  Children               ChildrenBySlot;
  BRepGraph_RevisionHash Hash;
  uint64_t               Key    = 0;
  size_t                 Count  = 0;
  bool                   IsLeaf = false;
};

//=================================================================================================

BRepGraph_RevisionMerkle::BRepGraph_RevisionMerkle()
    : myRootHash(emptyRootHash())
{
}

//=================================================================================================

BRepGraph_RevisionMerkle::BRepGraph_RevisionMerkle(const NodePtr&                theRoot,
                                                   const BRepGraph_RevisionHash& theRootHash)
    : myRoot(theRoot),
      myRootHash(theRootHash)
{
}

//=================================================================================================

BRepGraph_RevisionMerkle BRepGraph_RevisionMerkle::Build(NCollection_LinearVector<Entry> theEntries)
{
  if (theEntries.IsEmpty())
  {
    return BRepGraph_RevisionMerkle();
  }

  std::sort(theEntries.begin(), theEntries.end(), [](const Entry& theLeft, const Entry& theRight) {
    return theLeft.Key < theRight.Key;
  });
  for (size_t anIndex = 1; anIndex < theEntries.Size(); ++anIndex)
  {
    if (theEntries.Value(anIndex - 1).Key == theEntries.Value(anIndex).Key)
    {
      throw Standard_DomainError("BRepGraph_RevisionMerkle::Build: duplicate key");
    }
  }

  const NodePtr aRoot = build(theEntries, 0, theEntries.Size(), 0);
  return BRepGraph_RevisionMerkle(aRoot, aRoot->Hash);
}

//=================================================================================================

BRepGraph_RevisionMerkle BRepGraph_RevisionMerkle::Insert(
  const uint64_t                theKey,
  const BRepGraph_RevisionHash& theValue) const
{
  const NodePtr aRoot = insert(myRoot, theKey, theValue, 0);
  if (aRoot == myRoot)
  {
    return *this;
  }
  return BRepGraph_RevisionMerkle(aRoot, aRoot->Hash);
}

//=================================================================================================

BRepGraph_RevisionMerkle BRepGraph_RevisionMerkle::Remove(const uint64_t theKey) const
{
  const NodePtr aRoot = remove(myRoot, theKey, 0);
  if (aRoot == myRoot)
  {
    return *this;
  }
  return BRepGraph_RevisionMerkle(aRoot, aRoot == nullptr ? emptyRootHash() : aRoot->Hash);
}

//=================================================================================================

bool BRepGraph_RevisionMerkle::Contains(const uint64_t theKey) const
{
  return contains(myRoot, theKey, 0);
}

//=================================================================================================

size_t BRepGraph_RevisionMerkle::Size() const noexcept
{
  return myRoot == nullptr ? 0 : myRoot->Count;
}

//=================================================================================================

BRepGraph_RevisionMerkle::NodePtr BRepGraph_RevisionMerkle::insert(
  const NodePtr&                theNode,
  const uint64_t                theKey,
  const BRepGraph_RevisionHash& theValue,
  const int                     theDepth)
{
  if (theDepth == THE_KEY_BYTES)
  {
    const BRepGraph_RevisionHash aHash = leafHash(theKey, theValue);
    if (theNode != nullptr && theNode->IsLeaf && theNode->Key == theKey && theNode->Hash == aHash)
    {
      return theNode;
    }
    return std::make_shared<const Node>(theKey, aHash);
  }

  const uint8_t aSlot = keySlot(theKey, theDepth);
  const size_t  aChildIndex =
    theNode == nullptr ? 0 : Node::ChildIndex(theNode->ChildrenBySlot, aSlot);
  const bool    hasChild   = theNode != nullptr && aChildIndex < theNode->ChildrenBySlot.Size()
                             && theNode->ChildrenBySlot.Value(aChildIndex).Slot == aSlot;
  const NodePtr anOldChild = hasChild ? theNode->ChildrenBySlot.Value(aChildIndex).Node : NodePtr();
  const NodePtr aNewChild  = insert(anOldChild, theKey, theValue, theDepth + 1);
  if (aNewChild == anOldChild)
  {
    return theNode;
  }

  Node::Children aChildren = theNode == nullptr ? Node::Children() : theNode->ChildrenBySlot;
  if (hasChild)
  {
    aChildren.ChangeValue(aChildIndex).Node = aNewChild;
  }
  else
  {
    aChildren.InsertBefore(aChildIndex, {aSlot, aNewChild});
  }
  return std::make_shared<const Node>(aChildren, theDepth);
}

//=================================================================================================

BRepGraph_RevisionMerkle::NodePtr BRepGraph_RevisionMerkle::build(
  const NCollection_LinearVector<Entry>& theEntries,
  const size_t                           theFirst,
  const size_t                           theLast,
  const int                              theDepth)
{
  if (theDepth == THE_KEY_BYTES)
  {
    const Entry& anEntry = theEntries.Value(theFirst);
    return std::make_shared<const Node>(anEntry.Key, leafHash(anEntry.Key, anEntry.Value));
  }

  Node::Children aChildren;
  size_t         aFirst = theFirst;
  while (aFirst < theLast)
  {
    const uint8_t aSlot = keySlot(theEntries.Value(aFirst).Key, theDepth);
    size_t        aNext = aFirst + 1;
    while (aNext < theLast && keySlot(theEntries.Value(aNext).Key, theDepth) == aSlot)
    {
      ++aNext;
    }
    aChildren.Append({aSlot, build(theEntries, aFirst, aNext, theDepth + 1)});
    aFirst = aNext;
  }
  return std::make_shared<const Node>(aChildren, theDepth);
}

//=================================================================================================

BRepGraph_RevisionMerkle::NodePtr BRepGraph_RevisionMerkle::remove(const NodePtr& theNode,
                                                                   const uint64_t theKey,
                                                                   const int      theDepth)
{
  if (theNode == nullptr)
  {
    return theNode;
  }
  if (theDepth == THE_KEY_BYTES)
  {
    return theNode->IsLeaf && theNode->Key == theKey ? NodePtr() : theNode;
  }

  const uint8_t aSlot       = keySlot(theKey, theDepth);
  const size_t  aChildIndex = Node::ChildIndex(theNode->ChildrenBySlot, aSlot);
  if (aChildIndex == theNode->ChildrenBySlot.Size()
      || theNode->ChildrenBySlot.Value(aChildIndex).Slot != aSlot)
  {
    return theNode;
  }
  const NodePtr& anOldChild = theNode->ChildrenBySlot.Value(aChildIndex).Node;
  const NodePtr  aChild     = remove(anOldChild, theKey, theDepth + 1);
  if (aChild == anOldChild)
  {
    return theNode;
  }

  Node::Children aChildren = theNode->ChildrenBySlot;
  if (aChild != nullptr)
  {
    aChildren.ChangeValue(aChildIndex).Node = aChild;
    return std::make_shared<const Node>(aChildren, theDepth);
  }
  aChildren.Erase(aChildIndex);
  return aChildren.IsEmpty() ? NodePtr() : std::make_shared<const Node>(aChildren, theDepth);
}

//=================================================================================================

bool BRepGraph_RevisionMerkle::contains(const NodePtr& theNode,
                                        const uint64_t theKey,
                                        const int      theDepth)
{
  if (theNode == nullptr)
  {
    return false;
  }
  if (theDepth == THE_KEY_BYTES)
  {
    return theNode->IsLeaf && theNode->Key == theKey;
  }
  const uint8_t aSlot       = keySlot(theKey, theDepth);
  const size_t  aChildIndex = Node::ChildIndex(theNode->ChildrenBySlot, aSlot);
  if (aChildIndex == theNode->ChildrenBySlot.Size()
      || theNode->ChildrenBySlot.Value(aChildIndex).Slot != aSlot)
  {
    return false;
  }
  return contains(theNode->ChildrenBySlot.Value(aChildIndex).Node, theKey, theDepth + 1);
}

//=================================================================================================

BRepGraph_RevisionHash BRepGraph_RevisionMerkle::emptyRootHash()
{
  HashInput anInput;
  anInput.AppendUInt32(0x4d454d50u); // MEMP
  anInput.AppendUInt32(THE_FORMAT_VERSION);
  return anInput.Hash();
}
