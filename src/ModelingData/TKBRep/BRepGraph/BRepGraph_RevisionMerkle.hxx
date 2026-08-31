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

#ifndef _BRepGraph_RevisionMerkle_HeaderFile
#define _BRepGraph_RevisionMerkle_HeaderFile

#include <BRepGraph_RevisionHash.hxx>
#include <NCollection_LinearVector.hxx>
#include <Standard_Macro.hxx>

#include <cstddef>
#include <cstdint>
#include <memory>

//! Persistent sparse Merkle index used by BRepGraph revision hashing.
//!
//! Keys traverse a fixed-depth radix-256 tree from their most significant byte.
//! Insert() and Remove() return a new index while sharing every node not
//! on the changed path. Node hashes encode fields individually and never depend
//! on addresses or object representation.
//! Retained versions are safe for concurrent reads and independent persistent
//! updates. Assignment to the same object requires external synchronization.
class BRepGraph_RevisionMerkle
{
public:
  //! Key and value hash used to build an index in bulk.
  struct Entry
  {
    uint64_t               Key;
    BRepGraph_RevisionHash Value;
  };

  //! Construct an empty index.
  Standard_EXPORT BRepGraph_RevisionMerkle();

  //! Build a canonical index from arbitrarily ordered entries.
  //! Each final tree node is allocated once. Duplicate keys are rejected.
  //! @param[in] theEntries key and value hashes to store
  //! @return index with the same root hash as sequential insertion
  //! @throws Standard_DomainError if two entries have the same key
  [[nodiscard]] Standard_EXPORT static BRepGraph_RevisionMerkle Build(
    NCollection_LinearVector<Entry> theEntries);

  //! Return a new index containing the key and value hash.
  //! An existing key is updated without modifying this index.
  [[nodiscard]] Standard_EXPORT BRepGraph_RevisionMerkle
    Insert(uint64_t theKey, const BRepGraph_RevisionHash& theValue) const;

  //! Return a new index without the key. Removing an absent key is a no-op.
  [[nodiscard]] Standard_EXPORT BRepGraph_RevisionMerkle Remove(uint64_t theKey) const;

  //! Return true if the key is present.
  [[nodiscard]] Standard_EXPORT bool Contains(uint64_t theKey) const;

  //! Return the number of stored leaves in constant time.
  [[nodiscard]] Standard_EXPORT size_t Size() const noexcept;

  //! Return the cached canonical root hash in constant time.
  [[nodiscard]] const BRepGraph_RevisionHash& RootHash() const noexcept { return myRootHash; }

  //! Return true if the index has no leaves.
  [[nodiscard]] bool IsEmpty() const noexcept { return myRoot == nullptr; }

private:
  struct Node;
  using NodePtr = std::shared_ptr<const Node>;

  BRepGraph_RevisionMerkle(const NodePtr& theRoot, const BRepGraph_RevisionHash& theRootHash);

  [[nodiscard]] static NodePtr insert(const NodePtr&                theNode,
                                      uint64_t                      theKey,
                                      const BRepGraph_RevisionHash& theValue,
                                      int                           theDepth);
  [[nodiscard]] static NodePtr build(const NCollection_LinearVector<Entry>& theEntries,
                                     size_t                                 theFirst,
                                     size_t                                 theLast,
                                     int                                    theDepth);
  [[nodiscard]] static NodePtr remove(const NodePtr& theNode, uint64_t theKey, int theDepth);
  [[nodiscard]] static bool    contains(const NodePtr& theNode, uint64_t theKey, int theDepth);
  [[nodiscard]] static BRepGraph_RevisionHash emptyRootHash();

  NodePtr                myRoot;
  BRepGraph_RevisionHash myRootHash;
};

#endif // _BRepGraph_RevisionMerkle_HeaderFile
