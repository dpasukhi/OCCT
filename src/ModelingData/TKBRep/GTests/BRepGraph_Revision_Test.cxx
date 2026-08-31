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

#include <BRepGraph.hxx>
#include <BRepGraph_Compact.hxx>
#include <BRepGraph_Transaction.hxx>
#include <BRepGraph_EditorView.hxx>
#include <BRepGraph_Iterator.hxx>
#include <BRepGraph_LayerLock.hxx>
#include <BRepGraph_LayerRegistry.hxx>
#include <BRepGraph_RefsView.hxx>
#include <BRepGraph_Replace.hxx>
#include <BRepGraph_ShapesView.hxx>
#include <BRepGraphODE_RevisionPackage.hxx>
#include <BRepGraph_RevisionHash.hxx>
#include <BRepGraph_RevisionMerkle.hxx>
#include <BRepGraph_Revision.hxx>
#include <BRepGraph_TopoView.hxx>
#include <BRepGraph_UIDsView.hxx>

#include <BRepPrimAPI_MakeBox.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_Line.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_Triangle.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_TriangulationParameters.hxx>
#include <Standard_DomainError.hxx>
#include <TCollection_AsciiString.hxx>
#include <gp_Pnt.hxx>

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace
{

std::filesystem::path makePackageTestPath(const char* theName)
{
  const uint64_t aStamp =
    static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
  TCollection_AsciiString aName("occt-brepgraph-");
  aName += theName;
  aName += '-';
  aName += static_cast<int>(aStamp & 0x7fffffff);
  return std::filesystem::temp_directory_path() / aName.ToCString();
}

OSD_Path asOSDPath(const std::filesystem::path& thePath)
{
  return OSD_Path(TCollection_AsciiString(thePath.u8string().c_str()));
}

std::filesystem::path asFilesystemPath(const OSD_Path& thePath)
{
  TCollection_AsciiString aPath;
  thePath.SystemName(aPath);
  return std::filesystem::u8path(aPath.ToCString());
}

void expectFreshCoreHashes(const occ::handle<BRepGraph_Revision>& theRevision)
{
  ASSERT_FALSE(theRevision.IsNull());
  const BRepGraph_RevisionHash::Hasher::Result aFresh =
    BRepGraph_RevisionHash::Hasher::Compute(theRevision->Graph());
  EXPECT_EQ(theRevision->SemanticHash(), aFresh.Semantic);
  EXPECT_EQ(theRevision->StorageRootHash(), aFresh.Storage);
}

occ::handle<Geom_BSplineCurve> makeBSplineCurve(const double theMiddlePoleY)
{
  NCollection_Array1<gp_Pnt> aPoles(1, 3);
  aPoles.SetValue(1, gp_Pnt(0.0, 0.0, 0.0));
  aPoles.SetValue(2, gp_Pnt(0.5, theMiddlePoleY, 0.0));
  aPoles.SetValue(3, gp_Pnt(1.0, 0.0, 0.0));
  NCollection_Array1<double> aKnots(1, 2);
  aKnots.SetValue(1, 0.0);
  aKnots.SetValue(2, 1.0);
  NCollection_Array1<int> aMultiplicities(1, 2);
  aMultiplicities.SetValue(1, 3);
  aMultiplicities.SetValue(2, 3);
  return new Geom_BSplineCurve(aPoles, aKnots, aMultiplicities, 2);
}

BRepGraph_RevisionHash edgePayloadHash(const occ::handle<Geom_Curve>&     theCurve,
                                       const occ::handle<Poly_Polygon3D>& thePolygon)
{
  BRepGraph                aGraph;
  const BRepGraph_VertexId aStart = aGraph.Editor().Vertices().Add(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_VertexId anEnd  = aGraph.Editor().Vertices().Add(gp_Pnt(1.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_EdgeId   anEdge =
    aGraph.Editor().Edges().Add(aStart, anEnd, theCurve, 0.0, 1.0, 1.e-7);
  if (!anEdge.IsValid())
  {
    return BRepGraph_RevisionHash();
  }
  if (!thePolygon.IsNull())
  {
    aGraph.Editor().Edges().SetPersistentPolygon3D(anEdge, thePolygon);
  }
  return BRepGraph_RevisionHash::Hasher::Semantic(aGraph);
}

BRepGraph_RevisionHash triangulationPayloadHash(const double theThirdNodeZ,
                                                const bool   theHasNormals,
                                                const double theNormalZ = 1.0)
{
  BRepGraph                                  aGraph;
  NCollection_LinearVector<BRepGraph_WireId> anInnerWires;
  const BRepGraph_FaceId aFace = aGraph.Editor().Faces().Add(occ::handle<Geom_Surface>(),
                                                             BRepGraph_WireId(),
                                                             anInnerWires.ToArray1(),
                                                             1.e-7);
  if (!aFace.IsValid())
  {
    return BRepGraph_RevisionHash();
  }

  occ::handle<Poly_Triangulation> aTriangulation =
    new Poly_Triangulation(3, 1, true, theHasNormals);
  aTriangulation->Deflection(0.125);
  aTriangulation->SetMeshPurpose(Poly_MeshPurpose_Calculation);
  aTriangulation->SetNode(1, gp_Pnt(0.0, 0.0, 0.0));
  aTriangulation->SetNode(2, gp_Pnt(1.0, 0.0, 0.0));
  aTriangulation->SetNode(3, gp_Pnt(0.0, 1.0, theThirdNodeZ));
  aTriangulation->SetUVNode(1, gp_Pnt2d(0.0, 0.0));
  aTriangulation->SetUVNode(2, gp_Pnt2d(1.0, 0.0));
  aTriangulation->SetUVNode(3, gp_Pnt2d(0.0, 1.0));
  if (theHasNormals)
  {
    aTriangulation->SetNormal(1, gp_Dir(0.0, 0.0, theNormalZ));
    aTriangulation->SetNormal(2, gp_Dir(0.0, 0.0, theNormalZ));
    aTriangulation->SetNormal(3, gp_Dir(0.0, 0.0, theNormalZ));
  }
  aTriangulation->SetTriangle(1, Poly_Triangle(1, 2, 3));
  aTriangulation->Parameters(new Poly_TriangulationParameters(0.125, 0.5, 0.01));
  aGraph.Editor().Faces().SetPersistentTriangulation(aFace, aTriangulation);
  return BRepGraph_RevisionHash::Hasher::Semantic(aGraph);
}

} // namespace

TEST(BRepGraph_RevisionMerkleTest, InsertionOrderIsCanonical)
{
  const BRepGraph_RevisionHash aValue1 = BRepGraph_RevisionHash::Hasher::Bytes("one", 3);
  const BRepGraph_RevisionHash aValue2 = BRepGraph_RevisionHash::Hasher::Bytes("two", 3);
  const BRepGraph_RevisionHash aValue3 = BRepGraph_RevisionHash::Hasher::Bytes("three", 5);

  const BRepGraph_RevisionMerkle aForward = BRepGraph_RevisionMerkle()
                                              .Insert(0x0123456789abcdefull, aValue1)
                                              .Insert(0xfedcba9876543210ull, aValue2)
                                              .Insert(0x0123456789abcde0ull, aValue3);
  const BRepGraph_RevisionMerkle aReverse = BRepGraph_RevisionMerkle()
                                              .Insert(0x0123456789abcde0ull, aValue3)
                                              .Insert(0xfedcba9876543210ull, aValue2)
                                              .Insert(0x0123456789abcdefull, aValue1);

  EXPECT_EQ(aForward.RootHash(), aReverse.RootHash());
  EXPECT_EQ(aForward.Size(), 3u);

  const BRepGraph_RevisionMerkle anUnchanged = aForward.Insert(0x0123456789abcdefull, aValue1);
  EXPECT_EQ(anUnchanged.RootHash(), aForward.RootHash());
  EXPECT_EQ(anUnchanged.Size(), aForward.Size());

  const BRepGraph_RevisionMerkle aHighBits = BRepGraph_RevisionMerkle()
                                               .Insert(0x010000123456789aull, aValue1)
                                               .Insert(0x020000123456789aull, aValue2);
  EXPECT_EQ(aHighBits.Size(), 2u);
  EXPECT_TRUE(aHighBits.Contains(0x010000123456789aull));
  EXPECT_TRUE(aHighBits.Contains(0x020000123456789aull));
}

TEST(BRepGraph_RevisionHashTest, IncrementalBytesMatchContiguousBytes)
{
  std::string aPayload(128 * 1024 + 17, '\0');
  for (size_t anIndex = 0; anIndex < aPayload.size(); ++anIndex)
  {
    aPayload[anIndex] = static_cast<char>((anIndex * 37u + 11u) & 0xffu);
  }

  BRepGraph_RevisionHash::Hasher::ByteAccumulator anAccumulator(aPayload.size());
  ASSERT_TRUE(anAccumulator.Append(aPayload.data(), 23));
  ASSERT_TRUE(anAccumulator.Append(aPayload.data() + 23, 64 * 1024));
  ASSERT_TRUE(anAccumulator.Append(aPayload.data() + 23 + 64 * 1024,
                                   aPayload.size() - 23 - 64 * 1024));

  BRepGraph_RevisionHash anIncrementalHash;
  ASSERT_TRUE(anAccumulator.Finish(anIncrementalHash));
  EXPECT_EQ(anIncrementalHash,
            BRepGraph_RevisionHash::Hasher::Bytes(aPayload.data(), aPayload.size()));

  BRepGraph_RevisionHash::Hasher::ByteAccumulator aShortAccumulator(8);
  ASSERT_TRUE(aShortAccumulator.Append(aPayload.data(), 7));
  BRepGraph_RevisionHash aShortHash;
  EXPECT_FALSE(aShortAccumulator.Finish(aShortHash));
}

TEST(BRepGraph_RevisionMerkleTest, BulkBuildMatchesSequentialInsertionAndHandlesEmptyInput)
{
  NCollection_LinearVector<BRepGraph_RevisionMerkle::Entry> anEntries;
  anEntries.Append({0xfedcba9876543210ull, BRepGraph_RevisionHash::Hasher::Bytes("two", 3)});
  anEntries.Append({0x0123456789abcde0ull, BRepGraph_RevisionHash::Hasher::Bytes("three", 5)});
  anEntries.Append({0xffffffffffffffffull, BRepGraph_RevisionHash::Hasher::Bytes("last", 4)});
  anEntries.Append({0x0123456789abcdefull, BRepGraph_RevisionHash::Hasher::Bytes("one", 3)});
  anEntries.Append({0x0000000000000000ull, BRepGraph_RevisionHash::Hasher::Bytes("first", 5)});

  BRepGraph_RevisionMerkle aSequential;
  for (const BRepGraph_RevisionMerkle::Entry& anEntry : anEntries)
  {
    aSequential = aSequential.Insert(anEntry.Key, anEntry.Value);
  }
  const BRepGraph_RevisionMerkle aBulk = BRepGraph_RevisionMerkle::Build(anEntries);
  EXPECT_EQ(aBulk.RootHash(), aSequential.RootHash());
  EXPECT_EQ(aBulk.Size(), anEntries.Size());

  const NCollection_LinearVector<BRepGraph_RevisionMerkle::Entry> anEmptyEntries;
  const BRepGraph_RevisionMerkle anEmpty = BRepGraph_RevisionMerkle::Build(anEmptyEntries);
  EXPECT_TRUE(anEmpty.IsEmpty());
  EXPECT_EQ(anEmpty.Size(), 0u);
  EXPECT_EQ(anEmpty.RootHash(), BRepGraph_RevisionMerkle().RootHash());

  anEntries.Append(anEntries.First());
  EXPECT_THROW(
    {
      const BRepGraph_RevisionMerkle aDuplicate = BRepGraph_RevisionMerkle::Build(anEntries);
      (void)aDuplicate;
    },
    Standard_DomainError);
}

TEST(BRepGraph_RevisionMerkleTest, UpdatesAndRemovalPreserveBase)
{
  const uint64_t               aKey           = 0x123456789abcdef0ull;
  const BRepGraph_RevisionHash aValue         = BRepGraph_RevisionHash::Hasher::Bytes("base", 4);
  const BRepGraph_RevisionHash anUpdatedValue = BRepGraph_RevisionHash::Hasher::Bytes("updated", 7);
  const BRepGraph_RevisionMerkle aBase        = BRepGraph_RevisionMerkle().Insert(aKey, aValue);
  const BRepGraph_RevisionHash   aBaseHash    = aBase.RootHash();

  const BRepGraph_RevisionMerkle anUpdated = aBase.Insert(aKey, anUpdatedValue);
  EXPECT_EQ(aBase.Size(), 1u);
  EXPECT_EQ(anUpdated.Size(), 1u);
  EXPECT_TRUE(aBase.Contains(aKey));
  EXPECT_EQ(aBase.RootHash(), aBaseHash);
  EXPECT_NE(anUpdated.RootHash(), aBaseHash);

  const BRepGraph_RevisionMerkle aRemoved = anUpdated.Remove(aKey);
  EXPECT_FALSE(aRemoved.Contains(aKey));
  EXPECT_TRUE(aRemoved.IsEmpty());
  EXPECT_EQ(aRemoved.Size(), 0u);
  EXPECT_EQ(aRemoved.RootHash(), BRepGraph_RevisionMerkle().RootHash());
  EXPECT_TRUE(anUpdated.Contains(aKey));
}

TEST(BRepGraph_RevisionHashTest, BSplinePolePayloadChangesSemanticHash)
{
  const BRepGraph_RevisionHash aFirst  = edgePayloadHash(makeBSplineCurve(0.25), {});
  const BRepGraph_RevisionHash aSecond = edgePayloadHash(makeBSplineCurve(0.75), {});
  ASSERT_FALSE(aFirst.IsNull());
  ASSERT_FALSE(aSecond.IsNull());
  EXPECT_NE(aFirst, aSecond);
}

TEST(BRepGraph_RevisionHashTest, PolygonNodePayloadChangesSemanticHash)
{
  NCollection_Array1<gp_Pnt> aFirstNodes(1, 2);
  aFirstNodes.SetValue(1, gp_Pnt(0.0, 0.0, 0.0));
  aFirstNodes.SetValue(2, gp_Pnt(1.0, 0.0, 0.0));
  NCollection_Array1<gp_Pnt> aSecondNodes = aFirstNodes;
  aSecondNodes.SetValue(2, gp_Pnt(1.0, 0.5, 0.0));
  occ::handle<Poly_Polygon3D> aFirstPolygon  = new Poly_Polygon3D(aFirstNodes);
  occ::handle<Poly_Polygon3D> aSecondPolygon = new Poly_Polygon3D(aSecondNodes);
  aFirstPolygon->Deflection(0.01);
  aSecondPolygon->Deflection(0.01);

  const occ::handle<Geom_Curve> aCurve  = makeBSplineCurve(0.25);
  const BRepGraph_RevisionHash  aFirst  = edgePayloadHash(aCurve, aFirstPolygon);
  const BRepGraph_RevisionHash  aSecond = edgePayloadHash(aCurve, aSecondPolygon);
  ASSERT_FALSE(aFirst.IsNull());
  ASSERT_FALSE(aSecond.IsNull());
  EXPECT_NE(aFirst, aSecond);
}

TEST(BRepGraph_RevisionHashTest, TriangulationNodePayloadChangesSemanticHash)
{
  const BRepGraph_RevisionHash aFirst  = triangulationPayloadHash(0.0, true);
  const BRepGraph_RevisionHash aSecond = triangulationPayloadHash(0.25, true);
  ASSERT_FALSE(aFirst.IsNull());
  ASSERT_FALSE(aSecond.IsNull());
  EXPECT_NE(aFirst, aSecond);
}

TEST(BRepGraph_RevisionHashTest, TriangulationNormalsChangeSemanticHash)
{
  const BRepGraph_RevisionHash aWithoutNormals  = triangulationPayloadHash(0.0, false);
  const BRepGraph_RevisionHash aPositiveNormals = triangulationPayloadHash(0.0, true, 1.0);
  const BRepGraph_RevisionHash aNegativeNormals = triangulationPayloadHash(0.0, true, -1.0);
  ASSERT_FALSE(aWithoutNormals.IsNull());
  EXPECT_NE(aWithoutNormals, aPositiveNormals);
  EXPECT_NE(aPositiveNormals, aNegativeNormals);
}

TEST(BRepGraph_RevisionHashTest, OwnershipChangesOnlyPhysicalHashes)
{
  BRepGraph                aGraph;
  const BRepGraph_VertexId aStart = aGraph.Editor().Vertices().Add(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_VertexId anEnd  = aGraph.Editor().Vertices().Add(gp_Pnt(1.0, 0.0, 0.0), 1.e-7);
  const occ::handle<Geom_Curve> aCurve =
    new Geom_Line(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0));
  const BRepGraph_EdgeId anEdge =
    aGraph.Editor().Edges().Add(aStart, anEnd, aCurve, 0.0, 1.0, 1.e-7);
  ASSERT_TRUE(anEdge.IsValid());
  const BRepGraph_RefId aRef = aGraph.Topo().Edges().Definition(anEdge).StartVertexRefId;

  const BRepGraph_RevisionHash aNodeSemantic = BRepGraph_RevisionHash::Hasher::Node(aGraph, aStart);
  const BRepGraph_RevisionHash aRefSemantic =
    BRepGraph_RevisionHash::Hasher::Reference(aGraph, aRef);
  const BRepGraph_RevisionHash     aStorage = BRepGraph_RevisionHash::Hasher::Storage(aGraph);
  occ::handle<BRepGraph_LayerLock> aLock    = aGraph.LayerRegistry().Ensure<BRepGraph_LayerLock>();
  ASSERT_FALSE(aLock.IsNull());
  const Standard_GUID anOwner("7ad91d25-533c-4f20-b93b-dad463f52ed9");

  aLock->SetOwner(BRepGraph_NodeId(aStart), anOwner);
  EXPECT_EQ(BRepGraph_RevisionHash::Hasher::Node(aGraph, aStart), aNodeSemantic);
  EXPECT_NE(BRepGraph_RevisionHash::Hasher::Storage(aGraph), aStorage);
  aLock->UnsetOwner(BRepGraph_NodeId(aStart));
  EXPECT_EQ(BRepGraph_RevisionHash::Hasher::Storage(aGraph), aStorage);

  aLock->SetOwner(aRef, anOwner);
  EXPECT_EQ(BRepGraph_RevisionHash::Hasher::Reference(aGraph, aRef), aRefSemantic);
  EXPECT_NE(BRepGraph_RevisionHash::Hasher::Storage(aGraph), aStorage);
}

TEST(BRepGraph_RevisionHashTest, TombstonePayloadChangesOnlyPhysicalHash)
{
  BRepGraph                aFirstGraph;
  const BRepGraph_VertexId aFirstVertex =
    aFirstGraph.Editor().Vertices().Add(gp_Pnt(1.0, 2.0, 3.0), 1.e-7);
  ASSERT_TRUE(aFirstVertex.IsValid());
  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::FromGraph(aFirstGraph);
  ASSERT_FALSE(aRevision.IsNull());
  BRepGraph aSecondGraph;
  ASSERT_TRUE(aRevision->CopyTo(aSecondGraph));
  {
    BRepGraph_MutGuard<BRepGraphInc::VertexDef> aChange =
      aSecondGraph.Editor().Vertices().Mut(aFirstVertex);
    aSecondGraph.Editor().Vertices().SetPoint(aChange, gp_Pnt(4.0, 5.0, 6.0));
  }
  aFirstGraph.Editor().Gen().RemoveNode(aFirstVertex);
  aSecondGraph.Editor().Gen().RemoveNode(aFirstVertex);

  EXPECT_EQ(BRepGraph_RevisionHash::Hasher::Semantic(aFirstGraph),
            BRepGraph_RevisionHash::Hasher::Semantic(aSecondGraph));
  EXPECT_NE(BRepGraph_RevisionHash::Hasher::Storage(aFirstGraph),
            BRepGraph_RevisionHash::Hasher::Storage(aSecondGraph));
}

TEST(BRepGraph_RevisionTest, EmptyRevisionProvidesDirectReads)
{
  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::Empty();
  ASSERT_FALSE(aRevision.IsNull());
  EXPECT_TRUE(aRevision->IsEmpty());
  EXPECT_TRUE(aRevision->SupportsSparseEdits());
  EXPECT_EQ(aRevision->SemanticHash(),
            BRepGraph_RevisionHash::Hasher::Semantic(aRevision->Graph()));

  EXPECT_TRUE(aRevision->Graph().IsEmpty());
  EXPECT_EQ(aRevision->NbVisibleVertices(), 0u);
  const BRepGraph_RevisionDiff aSelfDiff = aRevision->Diff(*aRevision);
  EXPECT_TRUE(aSelfDiff.CreatedUIDs.IsEmpty());
  EXPECT_TRUE(aSelfDiff.ModifiedUIDs.IsEmpty());
  EXPECT_TRUE(aSelfDiff.RemovedUIDs.IsEmpty());
}

TEST(BRepGraph_RevisionTest, VisibleIteratorsYieldResolvedRecordsWithoutUIDCollections)
{
  BRepGraph aGraph;
  ASSERT_TRUE(aGraph.Shapes().Add(BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape()).IsOk());
  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aRevision.IsNull());

  const auto countVisible = [](auto theIterator) {
    uint32_t aCount = 0;
    for (const auto& aChange : theIterator)
    {
      EXPECT_TRUE(aChange.UID.IsValid());
      EXPECT_NE(aChange.Kind, BRepGraph_Revision::VertexChange::Operation::Remove);
      ++aCount;
    }
    return aCount;
  };

  EXPECT_EQ(countVisible(aRevision->VisibleVertices()), aRevision->NbVisibleVertices());
  EXPECT_EQ(countVisible(aRevision->VisibleEdges()), aRevision->NbVisibleEdges());
  EXPECT_EQ(countVisible(aRevision->VisibleVertexRefs()), aRevision->NbVisibleVertexRefs());
  EXPECT_EQ(countVisible(aRevision->VisibleCoEdges()), aRevision->NbVisibleCoEdges());
  EXPECT_EQ(countVisible(aRevision->VisibleWires()), aRevision->NbVisibleWires());
  EXPECT_EQ(countVisible(aRevision->VisibleFaces()), aRevision->NbVisibleFaces());
  EXPECT_EQ(countVisible(aRevision->VisibleWireRefs()), aRevision->NbVisibleWireRefs());

  BRepGraph_Revision::VertexIterator anIterator = aRevision->VisibleVertices();
  ASSERT_TRUE(anIterator.More());
  EXPECT_TRUE(anIterator.Current().LocalId.IsValid());
  anIterator.Next();
}

TEST(BRepGraph_RevisionTest, ImmutableRevisionSupportsIndependentConcurrentIterators)
{
  BRepGraph aGraph;
  ASSERT_TRUE(aGraph.Shapes().Add(BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape()).IsOk());
  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aRevision.IsNull());

  std::atomic<bool>        isValid{true};
  std::vector<std::thread> aThreads;
  for (int aThreadIndex = 0; aThreadIndex < 4; ++aThreadIndex)
  {
    aThreads.emplace_back([&]() {
      for (int anIteration = 0; anIteration < 32; ++anIteration)
      {
        uint32_t anEdgeCount = 0;
        for (const BRepGraph_Revision::EdgeView& anEdge : aRevision->VisibleEdges())
        {
          if (!anEdge.UID.IsValid() || anEdge.Curve == nullptr)
          {
            isValid = false;
          }
          ++anEdgeCount;
        }
        if (anEdgeCount != aRevision->NbVisibleEdges()
            || aRevision->Graph().Topo().Faces().NbActive() != aRevision->NbVisibleFaces())
        {
          isValid = false;
        }
      }
    });
  }
  for (std::thread& aThread : aThreads)
  {
    aThread.join();
  }
  EXPECT_TRUE(isValid);
}

TEST(BRepGraph_RevisionTest, RevisionIsIsolatedFromSourceGraphMutation)
{
  BRepGraph aGraph;
  ASSERT_TRUE(aGraph.Editor().Vertices().Add(gp_Pnt(1.0, 2.0, 3.0), 1.e-7).IsValid());

  const BRepGraph_Revision::CreateResult aCreate = BRepGraph_Revision::Create(aGraph);
  ASSERT_TRUE(aCreate.IsOk());
  EXPECT_EQ(aCreate.Revision->SemanticHash(),
            BRepGraph_RevisionHash::Hasher::Semantic(aCreate.Revision->Graph()));
  ASSERT_EQ(aCreate.Revision->Graph().Topo().Vertices().NbActive(), 1u);

  ASSERT_TRUE(aGraph.Editor().Vertices().Add(gp_Pnt(4.0, 5.0, 6.0), 1.e-7).IsValid());
  EXPECT_EQ(aGraph.Topo().Vertices().NbActive(), 2u);
  EXPECT_EQ(aCreate.Revision->Graph().Topo().Vertices().NbActive(), 1u);
}

TEST(BRepGraph_RevisionTest, CopyToCreatesMutablePageSharedGraph)
{
  BRepGraph                           aSource;
  const BRepGraph::ShapesView::Result aBuilt =
    aSource.Shapes().Add(BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape());
  ASSERT_TRUE(aBuilt.IsOk());
  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::FromGraph(aSource);
  ASSERT_FALSE(aRevision.IsNull());

  BRepGraph aWorkingGraph;
  ASSERT_TRUE(aRevision->CopyTo(aWorkingGraph));
  EXPECT_EQ(aWorkingGraph.Topo().Faces().Nb(), aSource.Topo().Faces().Nb());
  const TopoDS_Shape aSourceOriginal  = aSource.Shapes().Original(aBuilt.TopologyRoot);
  const TopoDS_Shape aRootOriginal    = aRevision->Graph().Shapes().Original(aBuilt.TopologyRoot);
  const TopoDS_Shape aWorkingOriginal = aWorkingGraph.Shapes().Original(aBuilt.TopologyRoot);
  ASSERT_FALSE(aSourceOriginal.IsNull());
  ASSERT_FALSE(aRootOriginal.IsNull());
  ASSERT_FALSE(aWorkingOriginal.IsNull());
  EXPECT_FALSE(aRootOriginal.IsSame(aSourceOriginal));
  EXPECT_FALSE(aWorkingOriginal.IsSame(aRootOriginal));
  aWorkingGraph.Editor().Gen().RemoveSubgraph(aBuilt.Product);

  EXPECT_EQ(aWorkingGraph.Topo().Products().NbActive(), 0u);
  EXPECT_EQ(aWorkingGraph.Topo().Occurrences().NbActive(), 0u);
  EXPECT_EQ(aWorkingGraph.Topo().Compounds().NbActive(), 0u);
  EXPECT_EQ(aWorkingGraph.Topo().CompSolids().NbActive(), 0u);
  EXPECT_EQ(aWorkingGraph.Topo().Solids().NbActive(), 0u);
  EXPECT_EQ(aWorkingGraph.Topo().Shells().NbActive(), 0u);
  EXPECT_EQ(aWorkingGraph.Topo().Faces().NbActive(), 0u);
  EXPECT_EQ(aWorkingGraph.Topo().Wires().NbActive(), 0u);
  EXPECT_EQ(aWorkingGraph.Topo().CoEdges().NbActive(), 0u);
  EXPECT_EQ(aWorkingGraph.Topo().Edges().NbActive(), 0u);
  EXPECT_EQ(aWorkingGraph.Topo().Vertices().NbActive(), 0u);
  EXPECT_FALSE(aRevision->IsEmpty());
  EXPECT_EQ(aRevision->Graph().Topo().Faces().Nb(), aSource.Topo().Faces().Nb());
}

TEST(BRepGraph_RevisionTest, CopiedMutablePayloadDoesNotModifyRevisionOrSource)
{
  BRepGraph                aSource;
  const BRepGraph_VertexId aStart = aSource.Editor().Vertices().Add(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_VertexId anEnd  = aSource.Editor().Vertices().Add(gp_Pnt(1.0, 0.0, 0.0), 1.e-7);
  const occ::handle<Geom_BSplineCurve> aSourceCurve = makeBSplineCurve(0.25);
  const BRepGraph_EdgeId               anEdge =
    aSource.Editor().Edges().Add(aStart, anEnd, aSourceCurve, 0.0, 1.0, 1.e-7);
  ASSERT_TRUE(anEdge.IsValid());

  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::FromGraph(aSource);
  ASSERT_FALSE(aRevision.IsNull());
  aSourceCurve->SetPole(2, gp_Pnt(0.5, 2.0, 0.0));

  BRepGraph aWorkingGraph;
  ASSERT_TRUE(aRevision->CopyTo(aWorkingGraph));
  const occ::handle<Geom_BSplineCurve> aWorkingCurve =
    occ::down_cast<Geom_BSplineCurve>(aWorkingGraph.Topo().Edges().Curve3D(anEdge));
  ASSERT_FALSE(aWorkingCurve.IsNull());
  EXPECT_DOUBLE_EQ(aWorkingCurve->Pole(2).Y(), 0.25);
  aWorkingCurve->SetPole(2, gp_Pnt(0.5, 3.0, 0.0));

  BRepGraph aFreshGraph;
  ASSERT_TRUE(aRevision->CopyTo(aFreshGraph));
  const occ::handle<Geom_BSplineCurve> aFreshCurve =
    occ::down_cast<Geom_BSplineCurve>(aFreshGraph.Topo().Edges().Curve3D(anEdge));
  ASSERT_FALSE(aFreshCurve.IsNull());
  EXPECT_DOUBLE_EQ(aFreshCurve->Pole(2).Y(), 0.25);
  EXPECT_DOUBLE_EQ(aSourceCurve->Pole(2).Y(), 2.0);
}

TEST(BRepGraph_RevisionTest, BranchPayloadMutationDoesNotModifyImmutableRevisions)
{
  BRepGraph                aGraph;
  const BRepGraph_VertexId aStart = aGraph.Editor().Vertices().Add(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_VertexId anEnd  = aGraph.Editor().Vertices().Add(gp_Pnt(1.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_EdgeId   anEdge =
    aGraph.Editor().Edges().Add(aStart, anEnd, makeBSplineCurve(0.25), 0.0, 1.0, 1.e-7);
  ASSERT_TRUE(anEdge.IsValid());
  const occ::handle<BRepGraph_Revision> aBase = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aBase.IsNull());
  const BRepGraph_UID anEdgeUID = aBase->Graph().UIDs().Of(anEdge);

  BRepGraph_Transaction anEdit = aBase->BeginTransaction();
  ASSERT_TRUE(anEdit.SetEdgeTolerance(anEdgeUID, 2.e-7));
  const BRepGraph_Transaction::CommitResult aCommit = anEdit.Commit();
  ASSERT_TRUE(aCommit.IsOk());

  BRepGraph aBranchGraph;
  ASSERT_TRUE(aCommit.Revision->CopyTo(aBranchGraph));
  const occ::handle<Geom_BSplineCurve> aBranchCurve =
    occ::down_cast<Geom_BSplineCurve>(aBranchGraph.Topo().Edges().Curve3D(anEdge));
  ASSERT_FALSE(aBranchCurve.IsNull());
  aBranchCurve->SetPole(2, gp_Pnt(0.5, 4.0, 0.0));

  BRepGraph aFreshBase;
  BRepGraph aFreshBranch;
  ASSERT_TRUE(aBase->CopyTo(aFreshBase));
  ASSERT_TRUE(aCommit.Revision->CopyTo(aFreshBranch));
  const occ::handle<Geom_BSplineCurve> aBaseCurve =
    occ::down_cast<Geom_BSplineCurve>(aFreshBase.Topo().Edges().Curve3D(anEdge));
  const occ::handle<Geom_BSplineCurve> aFreshBranchCurve =
    occ::down_cast<Geom_BSplineCurve>(aFreshBranch.Topo().Edges().Curve3D(anEdge));
  ASSERT_FALSE(aBaseCurve.IsNull());
  ASSERT_FALSE(aFreshBranchCurve.IsNull());
  EXPECT_DOUBLE_EQ(aBaseCurve->Pole(2).Y(), 0.25);
  EXPECT_DOUBLE_EQ(aFreshBranchCurve->Pole(2).Y(), 0.25);
  EXPECT_DOUBLE_EQ(aBase->Graph().Topo().Edges().Definition(anEdge).Tolerance, 1.e-7);
  EXPECT_DOUBLE_EQ(aCommit.Revision->Graph().Topo().Edges().Definition(anEdge).Tolerance, 2.e-7);
}

TEST(BRepGraph_RevisionTest, CommitCreatesPageSharedRevision)
{
  BRepGraph                aGraph;
  const BRepGraph_VertexId aBaseVertex =
    aGraph.Editor().Vertices().Add(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  ASSERT_TRUE(aBaseVertex.IsValid());
  const occ::handle<BRepGraph_Revision> aBase = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aBase.IsNull());

  BRepGraph_Transaction anEdit = aBase->BeginTransaction();
  ASSERT_TRUE(anEdit.IsValid()) << anEdit.Diagnostics().Size();
  const BRepGraph_VertexId aCreated = anEdit.AddVertex(gp_Pnt(10.0, 0.0, 0.0), 1.e-7);
  ASSERT_TRUE(aCreated.IsValid());

  const BRepGraph_Transaction::CommitResult aCommit = anEdit.Commit();
  ASSERT_TRUE(aCommit.IsOk());
  ASSERT_FALSE(aCommit.Revision.IsNull());
  EXPECT_NE(aCommit.Revision->StorageRootHash(), aBase->StorageRootHash());
  expectFreshCoreHashes(aCommit.Revision);
  EXPECT_EQ(aBase->Graph().Topo().Vertices().NbActive(), 1u);
  EXPECT_EQ(aCommit.Revision->Graph().Topo().Vertices().NbActive(), 2u);
  const BRepGraph_UID                   aCreatedUID = aCommit.Revision->Graph().UIDs().Of(aCreated);
  const occ::handle<BRepGraph_Revision> aRead       = aCommit.Revision;
  BRepGraph_Revision::VertexChange      aCreatedChange;
  ASSERT_TRUE(aRead->ReadVertex(aCreatedUID, aCreatedChange));
  EXPECT_DOUBLE_EQ(aCreatedChange.Definition.Point.X(), 10.0);
  EXPECT_EQ(aRead->NbVisibleVertices(), 2u);
  EXPECT_FALSE(aCommit.AllocatedUIDRanges.IsEmpty());
}

TEST(BRepGraph_RevisionTest, EdgeEditPreservesDurableEndpointsAndRelations)
{
  BRepGraph_Transaction aVertexEdit = BRepGraph_Revision::Empty()->BeginTransaction();
  ASSERT_TRUE(aVertexEdit.IsValid());
  const BRepGraph_VertexId aStartVertex = aVertexEdit.AddVertex(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_VertexId anEndVertex  = aVertexEdit.AddVertex(gp_Pnt(1.0, 0.0, 0.0), 1.e-7);
  ASSERT_TRUE(aStartVertex.IsValid() && anEndVertex.IsValid());
  const BRepGraph_Transaction::CommitResult aVertexCommit = aVertexEdit.Commit();
  ASSERT_TRUE(aVertexCommit.IsOk());
  const BRepGraph_UID aStartUID = aVertexCommit.Revision->Graph().UIDs().Of(aStartVertex);
  const BRepGraph_UID anEndUID  = aVertexCommit.Revision->Graph().UIDs().Of(anEndVertex);

  BRepGraph_Transaction anEdgeEdit = aVertexCommit.Revision->BeginTransaction();
  ASSERT_TRUE(anEdgeEdit.IsValid());
  const occ::handle<Geom_Curve> aCurve =
    new Geom_Line(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0));
  ASSERT_FALSE(aCurve.IsNull());
  const BRepGraph_EdgeId anEdge = anEdgeEdit.AddEdge(aStartUID, anEndUID, aCurve, 0.0, 1.0, 2.e-7);
  ASSERT_TRUE(anEdge.IsValid());
  const BRepGraph_Transaction::CommitResult anEdgeCommit = anEdgeEdit.Commit();
  ASSERT_TRUE(anEdgeCommit.IsOk());
  ASSERT_TRUE(anEdgeCommit.Revision->SupportsSparseEdits());
  EXPECT_EQ(anEdgeCommit.Revision->NbVisibleEdges(), 1u);
  EXPECT_EQ(anEdgeCommit.Revision->NbVisibleVertexRefs(), 2u);

  const BRepGraph_UID anEdgeUID               = anEdgeCommit.Revision->Graph().UIDs().Of(anEdge);
  const occ::handle<BRepGraph_Revision> aRead = anEdgeCommit.Revision;
  BRepGraph_Revision::EdgeChange        anEdgeChange;
  ASSERT_TRUE(aRead->ReadEdge(anEdgeUID, anEdgeChange));
  const BRepGraphInc::EdgeDef& anEdgeDefinition = anEdgeChange.Definition;
  EXPECT_EQ(aRead->NbVisibleVertexRefs(), 2u);
  const BRepGraph_RefUID aStartRefUID =
    anEdgeCommit.Revision->Graph().UIDs().Of(anEdgeDefinition.StartVertexRefId);
  BRepGraph_Revision::VertexRefChange aStartRefChange;
  ASSERT_TRUE(aRead->ReadVertexRef(aStartRefUID, aStartRefChange));
  EXPECT_EQ(aStartRefChange.Definition.ParentEdgeId, anEdge);
  const BRepGraph&               aReadGraph = aRead->Graph();
  const BRepGraphInc::VertexRef& aStartRef =
    aReadGraph.Refs().Vertices().Entry(anEdgeDefinition.StartVertexRefId);
  const BRepGraphInc::VertexRef& anEndRef =
    aReadGraph.Refs().Vertices().Entry(anEdgeDefinition.EndVertexRefId);
  EXPECT_EQ(aReadGraph.UIDs().Of(aStartRef.ChildVertexId), aStartUID);
  EXPECT_EQ(aReadGraph.UIDs().Of(anEndRef.ChildVertexId), anEndUID);
  EXPECT_DOUBLE_EQ(anEdgeDefinition.Tolerance, 2.e-7);
  EXPECT_TRUE(anEdgeCommit.Revision->Graph().ValidateRelations());
  EXPECT_EQ(anEdgeCommit.Revision->SemanticHash(),
            BRepGraph_RevisionHash::Hasher::Semantic(anEdgeCommit.Revision->Graph()));
  const BRepGraph_RevisionDiff anEdgeDiff = aVertexCommit.Revision->Diff(*anEdgeCommit.Revision);
  ASSERT_EQ(anEdgeDiff.CreatedUIDs.Size(), 1u);
  EXPECT_EQ(anEdgeDiff.CreatedUIDs.First(), anEdgeUID);
  EXPECT_EQ(anEdgeDiff.CreatedRefUIDs.Size(), 2u);

  const std::filesystem::path           anPackagePath = makePackageTestPath("edge");
  BRepGraphODE_RevisionPackage::Options anPackageOptions;
  const BRepGraphODE_RevisionPackage::Result anPackageWrite =
    BRepGraphODE_RevisionPackage::Write(*anEdgeCommit.Revision,
                                        asOSDPath(anPackagePath),
                                        anPackageOptions);
  ASSERT_TRUE(anPackageWrite.IsOk())
    << (anPackageWrite.Diagnostics.IsEmpty()
          ? ""
          : anPackageWrite.Diagnostics.First().Message.ToCString());
  const BRepGraphODE_RevisionPackage::Result anPackageRead =
    BRepGraphODE_RevisionPackage::Read(asOSDPath(anPackagePath), anPackageOptions);
  ASSERT_TRUE(anPackageRead.IsOk());
  EXPECT_EQ(anPackageRead.Revision->SemanticHash(), anEdgeCommit.Revision->SemanticHash());
  BRepGraph_Revision::EdgeChange anPackagedEdgeChange;
  EXPECT_TRUE(anPackageRead.Revision->ReadEdge(anEdgeUID, anPackagedEdgeChange));
  std::filesystem::remove_all(anPackagePath);

  BRepGraph_Transaction aToleranceEdit = anEdgeCommit.Revision->BeginTransaction();
  ASSERT_TRUE(aToleranceEdit.IsValid());
  ASSERT_TRUE(aToleranceEdit.SetEdgeTolerance(anEdgeUID, 3.e-7));
  const BRepGraph_Transaction::CommitResult aToleranceCommit = aToleranceEdit.Commit();
  ASSERT_TRUE(aToleranceCommit.IsOk());
  const occ::handle<Geom_Line> aPreservedCurve =
    occ::down_cast<Geom_Line>(aToleranceCommit.Revision->Graph().Topo().Edges().Curve3D(anEdge));
  ASSERT_FALSE(aPreservedCurve.IsNull());
  EXPECT_DOUBLE_EQ(aPreservedCurve->Position().Direction().X(), 1.0);
  EXPECT_DOUBLE_EQ(aPreservedCurve->Position().Direction().Y(), 0.0);

  BRepGraph_Transaction aModifyEdit = aToleranceCommit.Revision->BeginTransaction();
  ASSERT_TRUE(aModifyEdit.IsValid());
  const occ::handle<Geom_Curve> aUpdatedCurve =
    new Geom_Line(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 1.0, 0.0));
  ASSERT_TRUE(aModifyEdit.SetEdgeCurve(anEdgeUID, aUpdatedCurve, -1.0, 2.0));
  const BRepGraph_Transaction::CommitResult aModifyCommit = aModifyEdit.Commit();
  ASSERT_TRUE(aModifyCommit.IsOk());
  expectFreshCoreHashes(aModifyCommit.Revision);
  const occ::handle<Geom_Line> aCopiedCurve =
    occ::down_cast<Geom_Line>(aModifyCommit.Revision->Graph().Topo().Edges().Curve3D(anEdge));
  ASSERT_FALSE(aCopiedCurve.IsNull());
  EXPECT_DOUBLE_EQ(aCopiedCurve->Position().Direction().X(), 0.0);
  EXPECT_DOUBLE_EQ(aCopiedCurve->Position().Direction().Y(), 1.0);
  EXPECT_NE(aModifyCommit.Revision->SemanticHash(), aToleranceCommit.Revision->SemanticHash());
  BRepGraph_Revision::EdgeChange aModifiedChange;
  ASSERT_TRUE(aModifyCommit.Revision->ReadEdge(anEdgeUID, aModifiedChange));
  EXPECT_DOUBLE_EQ(aModifiedChange.Definition.Tolerance, 3.e-7);
  EXPECT_FALSE(aModifyCommit.Diff.ModifiedUIDs.IsEmpty());
  EXPECT_EQ(aModifyCommit.Diff.ModifiedUIDs.First(), anEdgeUID);
  const std::filesystem::path anUpdatedPackagePath = makePackageTestPath("edge-curve-update");
  BRepGraphODE_RevisionPackage::Options anUpdatedPackageOptions;
  ASSERT_TRUE(BRepGraphODE_RevisionPackage::Write(*aModifyCommit.Revision,
                                                  asOSDPath(anUpdatedPackagePath),
                                                  anUpdatedPackageOptions)
                .IsOk());
  const BRepGraphODE_RevisionPackage::Result anUpdatedPackageRead =
    BRepGraphODE_RevisionPackage::Read(asOSDPath(anUpdatedPackagePath), anUpdatedPackageOptions);
  ASSERT_TRUE(anUpdatedPackageRead.IsOk()) << anUpdatedPackageRead.Diagnostics.Size();
  const occ::handle<Geom_Line> anPackagedCurve = occ::down_cast<Geom_Line>(
    anUpdatedPackageRead.Revision->Graph().Topo().Edges().Curve3D(anEdge));
  ASSERT_FALSE(anPackagedCurve.IsNull());
  EXPECT_DOUBLE_EQ(anPackagedCurve->Position().Direction().X(), 0.0);
  EXPECT_DOUBLE_EQ(anPackagedCurve->Position().Direction().Y(), 1.0);
  std::filesystem::remove_all(anUpdatedPackagePath);

  BRepGraph_Transaction aRemoveEdit = aModifyCommit.Revision->BeginTransaction();
  ASSERT_TRUE(aRemoveEdit.IsValid());
  ASSERT_TRUE(aRemoveEdit.RemoveEdge(anEdgeUID));
  const BRepGraph_Transaction::CommitResult aRemoveCommit = aRemoveEdit.Commit();
  ASSERT_TRUE(aRemoveCommit.IsOk());
  expectFreshCoreHashes(aRemoveCommit.Revision);
  BRepGraph_Revision::EdgeChange aRemovedEdgeChange;
  EXPECT_FALSE(aRemoveCommit.Revision->ReadEdge(anEdgeUID, aRemovedEdgeChange));
  EXPECT_EQ(aRemoveCommit.Revision->NbVisibleEdges(), 0u);
  EXPECT_EQ(aRemoveCommit.Revision->NbVisibleVertexRefs(), 0u);
  EXPECT_TRUE(aRemoveCommit.Revision->Graph().ValidateRelations());
  EXPECT_FALSE(aRemoveCommit.Diff.RemovedRefUIDs.IsEmpty());
}

TEST(BRepGraph_RevisionTest, BulkEdgeCreationKeepsSequentialLocalIdsAndFreshHashes)
{
  BRepGraph_Transaction aVertexEdit = BRepGraph_Revision::Empty()->BeginTransaction();
  ASSERT_TRUE(aVertexEdit.IsValid());
  const BRepGraph_VertexId aStart = aVertexEdit.AddVertex(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_VertexId anEnd  = aVertexEdit.AddVertex(gp_Pnt(1.0, 0.0, 0.0), 1.e-7);
  ASSERT_TRUE(aStart.IsValid() && anEnd.IsValid());
  const BRepGraph_Transaction::CommitResult aVertexCommit = aVertexEdit.Commit();
  ASSERT_TRUE(aVertexCommit.IsOk());

  const BRepGraph_UID           aStartUID = aVertexCommit.Revision->Graph().UIDs().Of(aStart);
  const BRepGraph_UID           anEndUID  = aVertexCommit.Revision->Graph().UIDs().Of(anEnd);
  const occ::handle<Geom_Curve> aCurve =
    new Geom_Line(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0));
  BRepGraph_Transaction anEdgeEdit     = aVertexCommit.Revision->BeginTransaction();
  constexpr uint32_t    THE_EDGE_COUNT = 128;
  for (uint32_t anIndex = 0; anIndex < THE_EDGE_COUNT; ++anIndex)
  {
    EXPECT_EQ(anEdgeEdit.AddEdge(aStartUID, anEndUID, aCurve, 0.0, 1.0, 1.e-7),
              BRepGraph_EdgeId(anIndex));
  }

  const BRepGraph_Transaction::CommitResult anEdgeCommit = anEdgeEdit.Commit();
  ASSERT_TRUE(anEdgeCommit.IsOk());
  EXPECT_EQ(anEdgeCommit.Revision->NbVisibleEdges(), THE_EDGE_COUNT);
  EXPECT_EQ(anEdgeCommit.Revision->NbVisibleVertexRefs(), 2u * THE_EDGE_COUNT);
  expectFreshCoreHashes(anEdgeCommit.Revision);
}

TEST(BRepGraph_RevisionTest, CommitReplacesVertexRecordByDurableUID)
{
  BRepGraph                aGraph;
  const BRepGraph_VertexId aVertex = aGraph.Editor().Vertices().Add(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  ASSERT_TRUE(aVertex.IsValid());
  const occ::handle<BRepGraph_Revision> aBase = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aBase.IsNull());
  const BRepGraph_UID aUID = aBase->Graph().UIDs().Of(aVertex);

  BRepGraph_Transaction anEdit = aBase->BeginTransaction();
  ASSERT_TRUE(anEdit.IsValid());
  ASSERT_TRUE(anEdit.SetVertexPoint(aUID, gp_Pnt(1.0, 2.0, 3.0)));
  ASSERT_TRUE(anEdit.SetVertexTolerance(aUID, 2.e-6));

  const BRepGraph_Transaction::CommitResult aCommit = anEdit.Commit();
  ASSERT_TRUE(aCommit.IsOk());
  expectFreshCoreHashes(aCommit.Revision);
  const occ::handle<BRepGraph_Revision> aRead = aCommit.Revision;
  BRepGraph_Revision::VertexChange      aReadChange;
  ASSERT_TRUE(aRead->ReadVertex(aUID, aReadChange));
  EXPECT_DOUBLE_EQ(aReadChange.Definition.Point.X(), 1.0);
  EXPECT_DOUBLE_EQ(aReadChange.Definition.Tolerance, 2.e-6);
  EXPECT_NE(aCommit.Revision->SemanticHash(), aBase->SemanticHash());
  EXPECT_EQ(aCommit.Revision->SemanticHash(),
            BRepGraph_RevisionHash::Hasher::Semantic(aCommit.Revision->Graph()));
  const gp_Pnt& aChangedPoint =
    aCommit.Revision->Graph().Topo().Vertices().Definition(aVertex).Point;
  EXPECT_DOUBLE_EQ(aChangedPoint.X(), 1.0);
  EXPECT_DOUBLE_EQ(aChangedPoint.Y(), 2.0);
  EXPECT_DOUBLE_EQ(aChangedPoint.Z(), 3.0);
  EXPECT_DOUBLE_EQ(aCommit.Revision->Graph().Topo().Vertices().Definition(aVertex).Tolerance,
                   2.e-6);
  const gp_Pnt& aBasePoint = aBase->Graph().Topo().Vertices().Definition(aVertex).Point;
  EXPECT_DOUBLE_EQ(aBasePoint.X(), 0.0);
  EXPECT_DOUBLE_EQ(aBasePoint.Y(), 0.0);
  EXPECT_DOUBLE_EQ(aBasePoint.Z(), 0.0);
  EXPECT_FALSE(aCommit.Diff.ModifiedUIDs.IsEmpty());
  EXPECT_EQ(aCommit.Diff.ModifiedUIDs.First(), aUID);
}

TEST(BRepGraph_RevisionTest, FreeCoEdgeEditPreservesUsageIdentity)
{
  BRepGraph_Transaction aVertexEdit = BRepGraph_Revision::Empty()->BeginTransaction();
  ASSERT_TRUE(aVertexEdit.IsValid());
  const BRepGraph_VertexId aStartVertex = aVertexEdit.AddVertex(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_VertexId anEndVertex  = aVertexEdit.AddVertex(gp_Pnt(1.0, 0.0, 0.0), 1.e-7);
  ASSERT_TRUE(aStartVertex.IsValid() && anEndVertex.IsValid());
  const BRepGraph_Transaction::CommitResult aVertexCommit = aVertexEdit.Commit();
  ASSERT_TRUE(aVertexCommit.IsOk());
  const BRepGraph_UID aStartUID = aVertexCommit.Revision->Graph().UIDs().Of(aStartVertex);
  const BRepGraph_UID anEndUID  = aVertexCommit.Revision->Graph().UIDs().Of(anEndVertex);

  BRepGraph_Transaction anEdgeEdit = aVertexCommit.Revision->BeginTransaction();
  ASSERT_TRUE(anEdgeEdit.IsValid());
  const occ::handle<Geom_Curve> aCurve =
    new Geom_Line(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0));
  const BRepGraph_EdgeId anEdge = anEdgeEdit.AddEdge(aStartUID, anEndUID, aCurve, 0.0, 1.0, 2.e-7);
  ASSERT_TRUE(anEdge.IsValid());
  const BRepGraph_Transaction::CommitResult anEdgeCommit = anEdgeEdit.Commit();
  ASSERT_TRUE(anEdgeCommit.IsOk());
  const BRepGraph_UID anEdgeUID = anEdgeCommit.Revision->Graph().UIDs().Of(anEdge);

  BRepGraph_Transaction aCoEdgeEdit = anEdgeCommit.Revision->BeginTransaction();
  ASSERT_TRUE(aCoEdgeEdit.IsValid());
  const BRepGraph_CoEdgeId aCoEdge = aCoEdgeEdit.AddCoEdge(anEdgeUID, TopAbs_FORWARD);
  ASSERT_TRUE(aCoEdge.IsValid());
  const BRepGraph_Transaction::CommitResult aCoEdgeCommit = aCoEdgeEdit.Commit();
  ASSERT_TRUE(aCoEdgeCommit.IsOk());
  ASSERT_EQ(aCoEdgeCommit.Diff.UsageLinkChanges.Size(), 1u);
  EXPECT_EQ(aCoEdgeCommit.Diff.UsageLinkChanges.First().Kind,
            BRepGraph_RevisionDiff::ChangeKind::Created);
  expectFreshCoreHashes(aCoEdgeCommit.Revision);
  ASSERT_TRUE(aCoEdgeCommit.Revision->SupportsSparseEdits());
  EXPECT_EQ(aCoEdgeCommit.Revision->NbVisibleCoEdges(), 1u);

  const BRepGraph_UID aCoEdgeUID              = aCoEdgeCommit.Revision->Graph().UIDs().Of(aCoEdge);
  const occ::handle<BRepGraph_Revision> aRead = aCoEdgeCommit.Revision;
  BRepGraph_Revision::CoEdgeChange      aCoEdgeChange;
  ASSERT_TRUE(aRead->ReadCoEdge(aCoEdgeUID, aCoEdgeChange));
  EXPECT_EQ(aCoEdgeChange.Definition.ChildEdgeId, anEdge);
  EXPECT_EQ(aCoEdgeChange.Definition.Orientation, TopAbs_FORWARD);
  EXPECT_EQ(aRead->Graph().UIDs().Of(aCoEdgeChange.Definition.ChildEdgeId), anEdgeUID);
  EXPECT_TRUE(aCoEdgeCommit.Revision->Graph().ValidateRelations());
  EXPECT_EQ(aCoEdgeCommit.Revision->SemanticHash(),
            BRepGraph_RevisionHash::Hasher::Semantic(aCoEdgeCommit.Revision->Graph()));

  BRepGraph_Transaction anEdgeRemoveWhileUsed = aCoEdgeCommit.Revision->BeginTransaction();
  ASSERT_TRUE(anEdgeRemoveWhileUsed.IsValid());
  EXPECT_FALSE(anEdgeRemoveWhileUsed.RemoveEdge(anEdgeUID));
  anEdgeRemoveWhileUsed.Abort();

  const BRepGraph_RevisionDiff aCoEdgeDiff = anEdgeCommit.Revision->Diff(*aCoEdgeCommit.Revision);
  ASSERT_EQ(aCoEdgeDiff.CreatedUIDs.Size(), 1u);
  EXPECT_EQ(aCoEdgeDiff.CreatedUIDs.First(), aCoEdgeUID);
  ASSERT_EQ(aCoEdgeDiff.UsageLinkChanges.Size(), 1u);
  EXPECT_EQ(aCoEdgeDiff.UsageLinkChanges.First().LinkUID, aCoEdgeUID);
  EXPECT_EQ(aCoEdgeDiff.UsageLinkChanges.First().Kind, BRepGraph_RevisionDiff::ChangeKind::Created);

  const std::filesystem::path           anPackagePath = makePackageTestPath("coedge");
  BRepGraphODE_RevisionPackage::Options anPackageOptions;
  const BRepGraphODE_RevisionPackage::Result anPackageWrite =
    BRepGraphODE_RevisionPackage::Write(*aCoEdgeCommit.Revision,
                                        asOSDPath(anPackagePath),
                                        anPackageOptions);
  ASSERT_TRUE(anPackageWrite.IsOk())
    << (anPackageWrite.Diagnostics.IsEmpty()
          ? ""
          : anPackageWrite.Diagnostics.First().Message.ToCString());
  const BRepGraphODE_RevisionPackage::Result anPackageRead =
    BRepGraphODE_RevisionPackage::Read(asOSDPath(anPackagePath), anPackageOptions);
  ASSERT_TRUE(anPackageRead.IsOk());
  BRepGraph_Revision::CoEdgeChange anPackagedCoEdgeChange;
  EXPECT_TRUE(anPackageRead.Revision->ReadCoEdge(aCoEdgeUID, anPackagedCoEdgeChange));
  std::filesystem::remove_all(anPackagePath);

  BRepGraph_Transaction aModifyEdit = aCoEdgeCommit.Revision->BeginTransaction();
  ASSERT_TRUE(aModifyEdit.IsValid());
  ASSERT_TRUE(aModifyEdit.SetCoEdgeOrientation(aCoEdgeUID, TopAbs_REVERSED));
  const BRepGraph_Transaction::CommitResult aModifyCommit = aModifyEdit.Commit();
  ASSERT_TRUE(aModifyCommit.IsOk());
  ASSERT_EQ(aModifyCommit.Diff.UsageLinkChanges.Size(), 1u);
  EXPECT_EQ(aModifyCommit.Diff.UsageLinkChanges.First().Kind,
            BRepGraph_RevisionDiff::ChangeKind::Modified);
  expectFreshCoreHashes(aModifyCommit.Revision);
  BRepGraph_Revision::CoEdgeChange aModifiedCoEdgeChange;
  ASSERT_TRUE(aModifyCommit.Revision->ReadCoEdge(aCoEdgeUID, aModifiedCoEdgeChange));
  EXPECT_EQ(aModifiedCoEdgeChange.Definition.Orientation, TopAbs_REVERSED);
  EXPECT_EQ(aCoEdgeCommit.Revision->Diff(*aModifyCommit.Revision).UsageLinkChanges.First().Kind,
            BRepGraph_RevisionDiff::ChangeKind::Modified);

  BRepGraph_Transaction aRemoveEdit = aModifyCommit.Revision->BeginTransaction();
  ASSERT_TRUE(aRemoveEdit.IsValid());
  ASSERT_TRUE(aRemoveEdit.RemoveCoEdge(aCoEdgeUID));
  const BRepGraph_Transaction::CommitResult aRemoveCommit = aRemoveEdit.Commit();
  ASSERT_TRUE(aRemoveCommit.IsOk()) << (aRemoveCommit.Diagnostics.IsEmpty()
                                          ? ""
                                          : aRemoveCommit.Diagnostics.First().Message.ToCString());
  expectFreshCoreHashes(aRemoveCommit.Revision);
  ASSERT_EQ(aRemoveCommit.Diff.UsageLinkChanges.Size(), 1u);
  EXPECT_EQ(aRemoveCommit.Diff.UsageLinkChanges.First().Kind,
            BRepGraph_RevisionDiff::ChangeKind::Removed);
  BRepGraph_Revision::CoEdgeChange aRemovedCoEdgeChange;
  EXPECT_FALSE(aRemoveCommit.Revision->ReadCoEdge(aCoEdgeUID, aRemovedCoEdgeChange));
  EXPECT_EQ(aRemoveCommit.Revision->NbVisibleCoEdges(), 0u);
  EXPECT_TRUE(aRemoveCommit.Revision->Graph().ValidateRelations());
  const BRepGraph_RevisionDiff aRemoveDiff = aModifyCommit.Revision->Diff(*aRemoveCommit.Revision);
  ASSERT_EQ(aRemoveDiff.UsageLinkChanges.Size(), 1u);
  EXPECT_EQ(aRemoveDiff.UsageLinkChanges.First().Kind, BRepGraph_RevisionDiff::ChangeKind::Removed);
}

TEST(BRepGraph_RevisionTest, ImmutableRevisionReadDoesNotShareEdgeCurve)
{
  BRepGraph_Transaction aVertexEdit = BRepGraph_Revision::Empty()->BeginTransaction();
  ASSERT_TRUE(aVertexEdit.IsValid());
  const BRepGraph_VertexId aStartVertex = aVertexEdit.AddVertex(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_VertexId anEndVertex  = aVertexEdit.AddVertex(gp_Pnt(1.0, 0.0, 0.0), 1.e-7);
  ASSERT_TRUE(aStartVertex.IsValid() && anEndVertex.IsValid());
  const BRepGraph_Transaction::CommitResult aVertexCommit = aVertexEdit.Commit();
  ASSERT_TRUE(aVertexCommit.IsOk());

  const BRepGraph_UID   aStartUID  = aVertexCommit.Revision->Graph().UIDs().Of(aStartVertex);
  const BRepGraph_UID   anEndUID   = aVertexCommit.Revision->Graph().UIDs().Of(anEndVertex);
  BRepGraph_Transaction anEdgeEdit = aVertexCommit.Revision->BeginTransaction();
  ASSERT_TRUE(anEdgeEdit.IsValid());
  const occ::handle<Geom_Curve> aCurve =
    new Geom_Line(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0));
  const BRepGraph_EdgeId anEdge = anEdgeEdit.AddEdge(aStartUID, anEndUID, aCurve, 0.0, 1.0, 1.e-7);
  ASSERT_TRUE(anEdge.IsValid());
  const BRepGraph_Transaction::CommitResult anEdgeCommit = anEdgeEdit.Commit();
  ASSERT_TRUE(anEdgeCommit.IsOk());
  const BRepGraph_UID anEdgeUID = anEdgeCommit.Revision->Graph().UIDs().Of(anEdge);

  BRepGraph_Revision::EdgeChange aReadChange;
  ASSERT_TRUE(anEdgeCommit.Revision->ReadEdge(anEdgeUID, aReadChange));
  ASSERT_FALSE(aReadChange.Curve.IsNull());
  aReadChange.Curve->Reverse();

  BRepGraph_Revision::EdgeChange aFreshChange;
  ASSERT_TRUE(anEdgeCommit.Revision->ReadEdge(anEdgeUID, aFreshChange));
  ASSERT_FALSE(aFreshChange.Curve.IsNull());
  EXPECT_DOUBLE_EQ(aFreshChange.Curve->Value(1.0).X(), 1.0);

  BRepGraph_Revision::EdgeIterator anIterator = anEdgeCommit.Revision->VisibleEdges();
  ASSERT_TRUE(anIterator.More());
  ASSERT_NE(anIterator.Current().Curve, nullptr);
  EXPECT_DOUBLE_EQ(anIterator.Current().Curve->Value(1.0).X(), 1.0);
  ASSERT_TRUE(anEdgeCommit.Revision->ReadEdge(anEdgeUID, aFreshChange));
  EXPECT_DOUBLE_EQ(aFreshChange.Curve->Value(1.0).X(), 1.0);

  const BRepGraph&               aGraphCopy   = anEdgeCommit.Revision->Graph();
  const occ::handle<Geom_Curve>& aCopiedCurve = aGraphCopy.Topo().Edges().Curve3D(anEdge);
  ASSERT_FALSE(aCopiedCurve.IsNull());
  aCopiedCurve->Reverse();
  ASSERT_TRUE(anEdgeCommit.Revision->ReadEdge(anEdgeUID, aFreshChange));
  EXPECT_DOUBLE_EQ(aFreshChange.Curve->Value(1.0).X(), 1.0);
}

TEST(BRepGraph_RevisionTest, PackagePreservesEdgeAndCoEdgeTombstones)
{
  BRepGraph_Transaction aVertexEdit = BRepGraph_Revision::Empty()->BeginTransaction();
  ASSERT_TRUE(aVertexEdit.IsValid());
  const BRepGraph_VertexId aStartVertex = aVertexEdit.AddVertex(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_VertexId anEndVertex  = aVertexEdit.AddVertex(gp_Pnt(1.0, 0.0, 0.0), 1.e-7);
  ASSERT_TRUE(aStartVertex.IsValid() && anEndVertex.IsValid());
  const BRepGraph_Transaction::CommitResult aVertexCommit = aVertexEdit.Commit();
  ASSERT_TRUE(aVertexCommit.IsOk());
  const BRepGraph_UID aStartUID = aVertexCommit.Revision->Graph().UIDs().Of(aStartVertex);
  const BRepGraph_UID anEndUID  = aVertexCommit.Revision->Graph().UIDs().Of(anEndVertex);

  BRepGraph_Transaction anEdgeEdit = aVertexCommit.Revision->BeginTransaction();
  ASSERT_TRUE(anEdgeEdit.IsValid());
  const occ::handle<Geom_Curve> aCurve =
    new Geom_Line(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0));
  const BRepGraph_EdgeId anEdge = anEdgeEdit.AddEdge(aStartUID, anEndUID, aCurve, 0.0, 1.0, 1.e-7);
  ASSERT_TRUE(anEdge.IsValid());
  const BRepGraph_Transaction::CommitResult anEdgeCommit = anEdgeEdit.Commit();
  ASSERT_TRUE(anEdgeCommit.IsOk());
  const BRepGraph_UID anEdgeUID = anEdgeCommit.Revision->Graph().UIDs().Of(anEdge);

  BRepGraph_Transaction aCoEdgeEdit = anEdgeCommit.Revision->BeginTransaction();
  ASSERT_TRUE(aCoEdgeEdit.IsValid());
  const BRepGraph_CoEdgeId aCoEdge = aCoEdgeEdit.AddCoEdge(anEdgeUID, TopAbs_FORWARD);
  ASSERT_TRUE(aCoEdge.IsValid());
  const BRepGraph_Transaction::CommitResult aCoEdgeCommit = aCoEdgeEdit.Commit();
  ASSERT_TRUE(aCoEdgeCommit.IsOk());
  const BRepGraph_UID aCoEdgeUID = aCoEdgeCommit.Revision->Graph().UIDs().Of(aCoEdge);

  BRepGraph_Transaction aRemoveEdit = aCoEdgeCommit.Revision->BeginTransaction();
  ASSERT_TRUE(aRemoveEdit.IsValid());
  ASSERT_TRUE(aRemoveEdit.RemoveCoEdge(aCoEdgeUID));
  ASSERT_TRUE(aRemoveEdit.RemoveEdge(anEdgeUID));
  const BRepGraph_Transaction::CommitResult aRemoveCommit = aRemoveEdit.Commit();
  ASSERT_TRUE(aRemoveCommit.IsOk());
  BRepGraph_Revision::EdgeChange   aRemovedEdgeChange;
  BRepGraph_Revision::CoEdgeChange aRemovedCoEdgeChange;
  EXPECT_FALSE(aRemoveCommit.Revision->ReadEdge(anEdgeUID, aRemovedEdgeChange));
  EXPECT_FALSE(aRemoveCommit.Revision->ReadCoEdge(aCoEdgeUID, aRemovedCoEdgeChange));
  EXPECT_TRUE(aRemoveCommit.Revision->Graph().ValidateRelations());

  const BRepGraph_RevisionDiff aDiff = aCoEdgeCommit.Revision->Diff(*aRemoveCommit.Revision);
  EXPECT_TRUE(aDiff.RemovedUIDs.Size() >= 2u);
  EXPECT_TRUE(aDiff.RemovedRefUIDs.Size() >= 2u);
  ASSERT_EQ(aDiff.UsageLinkChanges.Size(), 1u);
  EXPECT_EQ(aDiff.UsageLinkChanges.First().LinkUID, aCoEdgeUID);
  EXPECT_EQ(aDiff.UsageLinkChanges.First().Kind, BRepGraph_RevisionDiff::ChangeKind::Removed);

  const std::filesystem::path aPath = makePackageTestPath("edge-coedge-tombstone");
  ASSERT_TRUE(
    BRepGraphODE_RevisionPackage::Write(*aRemoveCommit.Revision, asOSDPath(aPath)).IsOk());
  const BRepGraphODE_RevisionPackage::Result aRead =
    BRepGraphODE_RevisionPackage::Read(asOSDPath(aPath));
  ASSERT_TRUE(aRead.IsOk()) << aRead.Diagnostics.Size();
  ASSERT_FALSE(aRead.Revision.IsNull());
  EXPECT_TRUE(aRead.Revision->SupportsSparseEdits());
  EXPECT_TRUE(aRead.Revision->BeginTransaction().IsValid());

  BRepGraph_Transaction         aRestoredEdit = aRead.Revision->BeginTransaction();
  const occ::handle<Geom_Curve> aRestoredCurve =
    new Geom_Line(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0));
  ASSERT_TRUE(
    aRestoredEdit.AddEdge(aStartUID, anEndUID, aRestoredCurve, 0.0, 1.0, 1.e-7).IsValid());
  const BRepGraph_Transaction::CommitResult aRestoredCommit = aRestoredEdit.Commit();
  ASSERT_TRUE(aRestoredCommit.IsOk());
  EXPECT_EQ(aRestoredCommit.Revision->NbVisibleEdges(), 1u);
  EXPECT_EQ(aRestoredCommit.Revision->NbVisibleVertexRefs(), 2u);
  std::filesystem::remove_all(aPath);
}

TEST(BRepGraph_RevisionTest, AbortDoesNotChangeBaseRevision)
{
  BRepGraph                aGraph;
  const BRepGraph_VertexId aVertex = aGraph.Editor().Vertices().Add(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  ASSERT_TRUE(aVertex.IsValid());
  const occ::handle<BRepGraph_Revision> aBase = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aBase.IsNull());
  const BRepGraph_RevisionHash aHash = aBase->SemanticHash();

  BRepGraph_Transaction anEdit = aBase->BeginTransaction();
  ASSERT_TRUE(anEdit.IsValid());
  const BRepGraph_UID aUID = aBase->Graph().UIDs().Of(aVertex);
  ASSERT_TRUE(anEdit.SetVertexPoint(aUID, gp_Pnt(2.0, 0.0, 0.0)));
  anEdit.Abort();

  EXPECT_TRUE(anEdit.IsFinished());
  EXPECT_EQ(aBase->SemanticHash(), aHash);
  const gp_Pnt& aBasePoint = aBase->Graph().Topo().Vertices().Definition(aVertex).Point;
  EXPECT_DOUBLE_EQ(aBasePoint.X(), 0.0);
  EXPECT_DOUBLE_EQ(aBasePoint.Y(), 0.0);
  EXPECT_DOUBLE_EQ(aBasePoint.Z(), 0.0);
}

TEST(BRepGraph_RevisionTest, RemovalCreatesTombstoneAndRetainsBaseRecord)
{
  BRepGraph                aGraph;
  const BRepGraph_VertexId aVertex = aGraph.Editor().Vertices().Add(gp_Pnt(4.0, 5.0, 6.0), 1.e-7);
  ASSERT_TRUE(aVertex.IsValid());
  const occ::handle<BRepGraph_Revision> aBase = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aBase.IsNull());
  const BRepGraph_UID aUID = aBase->Graph().UIDs().Of(aVertex);

  BRepGraph_Transaction anEdit = aBase->BeginTransaction();
  ASSERT_TRUE(anEdit.IsValid());
  ASSERT_TRUE(anEdit.RemoveVertex(aUID));
  const BRepGraph_Transaction::CommitResult aCommit = anEdit.Commit();
  ASSERT_TRUE(aCommit.IsOk());

  BRepGraph_Revision::VertexChange aVertexChange;
  EXPECT_TRUE(aBase->ReadVertex(aUID, aVertexChange));
  EXPECT_FALSE(aCommit.Revision->ReadVertex(aUID, aVertexChange));
  EXPECT_EQ(aCommit.Revision->NbVisibleVertices(), 0u);
  EXPECT_FALSE(aCommit.Revision->VisibleVertices().More());
  EXPECT_FALSE(aCommit.Diff.RemovedUIDs.IsEmpty());
  EXPECT_EQ(aCommit.Diff.RemovedUIDs.First(), aUID);
}

TEST(BRepGraph_RevisionTest, CompleteGraphTransactionUsesExistingEditorAPI)
{
  BRepGraph aGraph;
  ASSERT_TRUE(aGraph.Shapes().Add(BRepPrimAPI_MakeBox(2.0, 3.0, 4.0).Shape()).IsOk());
  const occ::handle<BRepGraph_Revision> aBaseRevision = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aBaseRevision.IsNull());
  ASSERT_FALSE(aBaseRevision->SupportsSparseEdits());
  const uint32_t aBaseVertexCount = aBaseRevision->Graph().Topo().Vertices().NbActive();

  BRepGraph_Transaction aTransaction = aBaseRevision->BeginTransaction();
  ASSERT_TRUE(aTransaction.IsValid());
  BRepGraph* aWorkingGraph = aTransaction.Graph();
  ASSERT_NE(aWorkingGraph, nullptr);
  const BRepGraph_VertexId aVertex =
    aWorkingGraph->Editor().Vertices().Add(gp_Pnt(5.0, 6.0, 7.0), 1.e-7);
  ASSERT_TRUE(aVertex.IsValid());
  const BRepGraph_UID aVertexUID = aWorkingGraph->UIDs().Of(aVertex);
  ASSERT_TRUE(aVertexUID.IsValid());

  const BRepGraph_Transaction::CommitResult aCommit = aTransaction.Commit();
  ASSERT_TRUE(aCommit.IsOk());
  ASSERT_FALSE(aCommit.Revision.IsNull());
  EXPECT_NE(aCommit.Revision, aBaseRevision);
  EXPECT_EQ(aBaseRevision->Graph().Topo().Vertices().NbActive(), aBaseVertexCount);
  EXPECT_EQ(aCommit.Revision->Graph().Topo().Vertices().NbActive(), aBaseVertexCount + 1);
  EXPECT_EQ(aCommit.Diff.CreatedUIDs.Size(), 1u);
  EXPECT_EQ(aCommit.Diff.CreatedUIDs.First(), aVertexUID);
}

TEST(BRepGraph_RevisionTest, CompleteGraphTransactionDiffReportsModifyNoOpAndRemove)
{
  BRepGraph                aGraph;
  const BRepGraph_VertexId aVertex =
    aGraph.Editor().Vertices().Add(gp_Pnt(1.0, 2.0, 3.0), 1.e-7);
  ASSERT_TRUE(aVertex.IsValid());
  const occ::handle<BRepGraph_Revision> aBase = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aBase.IsNull());
  const BRepGraph_UID aUID = aBase->Graph().UIDs().Of(aVertex);

  BRepGraph_Transaction anEdit = aBase->BeginTransaction();
  BRepGraph*            aWorkingGraph = anEdit.Graph();
  ASSERT_NE(aWorkingGraph, nullptr);
  aWorkingGraph->Editor().Vertices().SetPoint(aVertex, gp_Pnt(4.0, 5.0, 6.0));
  const BRepGraph_Transaction::CommitResult aModified = anEdit.Commit();
  ASSERT_TRUE(aModified.IsOk());
  ASSERT_EQ(aModified.Diff.ModifiedUIDs.Size(), 1u);
  EXPECT_EQ(aModified.Diff.ModifiedUIDs.First(), aUID);

  BRepGraph_Transaction aNoOpEdit = aModified.Revision->BeginTransaction();
  BRepGraph*            aNoOpGraph = aNoOpEdit.Graph();
  ASSERT_NE(aNoOpGraph, nullptr);
  aNoOpGraph->Editor().Vertices().SetPoint(aVertex, gp_Pnt(4.0, 5.0, 6.0));
  const BRepGraph_Transaction::CommitResult aNoOp = aNoOpEdit.Commit();
  ASSERT_TRUE(aNoOp.IsOk());
  EXPECT_TRUE(aNoOp.Diff.IsEmpty());
  EXPECT_EQ(aNoOp.Revision, aModified.Revision);

  BRepGraph_Transaction aRemoveEdit = aModified.Revision->BeginTransaction();
  BRepGraph*            aRemoveGraph = aRemoveEdit.Graph();
  ASSERT_NE(aRemoveGraph, nullptr);
  aRemoveGraph->Editor().Gen().RemoveNode(aVertex);
  const BRepGraph_Transaction::CommitResult aRemoved = aRemoveEdit.Commit();
  ASSERT_TRUE(aRemoved.IsOk());
  ASSERT_EQ(aRemoved.Diff.RemovedUIDs.Size(), 1u);
  EXPECT_EQ(aRemoved.Diff.RemovedUIDs.First(), aUID);
}

TEST(BRepGraph_RevisionTest, UIDWatermarksOnlyRaiseAndReportReservedRanges)
{
  const occ::handle<BRepGraph_Revision> aBase = BRepGraph_Revision::Empty();
  ASSERT_FALSE(aBase.IsNull());

  BRepGraph_Transaction anEdit = aBase->BeginTransaction();
  ASSERT_TRUE(anEdit.IsValid());
  EXPECT_TRUE(anEdit.RaiseVertexUIDWatermark(100u));
  EXPECT_FALSE(anEdit.RaiseVertexUIDWatermark(50u));

  const BRepGraph_VertexId aVertex = anEdit.AddVertex(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  ASSERT_TRUE(aVertex.IsValid());

  const BRepGraph_Transaction::CommitResult aCommit = anEdit.Commit();
  ASSERT_TRUE(aCommit.IsOk());
  ASSERT_EQ(aCommit.AllocatedUIDRanges.Size(), 2);
  EXPECT_EQ(aCommit.AllocatedUIDRanges.First().First, 1u);
  EXPECT_EQ(aCommit.AllocatedUIDRanges.First().Last, 99u);
  EXPECT_EQ(aCommit.AllocatedUIDRanges.Last().First, 100u);
  EXPECT_EQ(aCommit.AllocatedUIDRanges.Last().Last, 100u);
  EXPECT_EQ(aCommit.Revision->Graph().UIDs().Of(aVertex).Counter, 100u);
}

TEST(BRepGraph_RevisionTest, WatermarkOnlyCommitRetainsCounterAdvance)
{
  const occ::handle<BRepGraph_Revision> aBase = BRepGraph_Revision::Empty();
  ASSERT_FALSE(aBase.IsNull());

  BRepGraph_Transaction anEdit = aBase->BeginTransaction();
  ASSERT_TRUE(anEdit.IsValid());
  ASSERT_TRUE(anEdit.RaiseVertexUIDWatermark(100u));
  const BRepGraph_Transaction::CommitResult aCommit = anEdit.Commit();
  ASSERT_TRUE(aCommit.IsOk());
  ASSERT_FALSE(aCommit.Revision.IsNull());
  EXPECT_NE(aCommit.Revision, aBase);
  EXPECT_EQ(aCommit.Revision->SemanticHash(), aBase->SemanticHash());
  EXPECT_NE(aCommit.Revision->StorageRootHash(), aBase->StorageRootHash());
  ASSERT_EQ(aCommit.AllocatedUIDRanges.Size(), 1u);
  EXPECT_EQ(aCommit.AllocatedUIDRanges.First().First, 1u);
  EXPECT_EQ(aCommit.AllocatedUIDRanges.First().Last, 99u);

  BRepGraph_Transaction aFollowup = aCommit.Revision->BeginTransaction();
  ASSERT_TRUE(aFollowup.IsValid());
  const BRepGraph_VertexId aVertex = aFollowup.AddVertex(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  ASSERT_TRUE(aVertex.IsValid());
  const BRepGraph_Transaction::CommitResult aFollowupCommit = aFollowup.Commit();
  ASSERT_TRUE(aFollowupCommit.IsOk());
  EXPECT_EQ(aFollowupCommit.Revision->Graph().UIDs().Of(aVertex).Counter, 100u);
}

TEST(BRepGraph_RevisionTest, CommitRemovalSupersedesPriorModification)
{
  BRepGraph                aGraph;
  const BRepGraph_VertexId aVertex = aGraph.Editor().Vertices().Add(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  ASSERT_TRUE(aVertex.IsValid());
  const occ::handle<BRepGraph_Revision> aBase = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aBase.IsNull());
  const BRepGraph_UID aUID = aBase->Graph().UIDs().Of(aVertex);

  BRepGraph_Transaction anEdit = aBase->BeginTransaction();
  ASSERT_TRUE(anEdit.SetVertexPoint(aUID, gp_Pnt(1.0, 2.0, 3.0)));
  ASSERT_TRUE(anEdit.RemoveVertex(aUID));
  const BRepGraph_Transaction::CommitResult aCommit = anEdit.Commit();
  ASSERT_TRUE(aCommit.IsOk());
  EXPECT_TRUE(aCommit.Diff.ModifiedUIDs.IsEmpty());
  ASSERT_EQ(aCommit.Diff.RemovedUIDs.Size(), 1u);
  EXPECT_EQ(aCommit.Diff.RemovedUIDs.First(), aUID);
}

TEST(BRepGraph_RevisionTest, SparseCommitOmitsNoOpModificationAndReleasesState)
{
  BRepGraph                aGraph;
  const gp_Pnt             aPoint(1.0, 2.0, 3.0);
  const BRepGraph_VertexId aVertex = aGraph.Editor().Vertices().Add(aPoint, 1.e-7);
  ASSERT_TRUE(aVertex.IsValid());
  const occ::handle<BRepGraph_Revision> aBase = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aBase.IsNull());
  const BRepGraph_UID aUID = aBase->Graph().UIDs().Of(aVertex);

  BRepGraph_Transaction anEdit = aBase->BeginTransaction();
  ASSERT_TRUE(anEdit.SetVertexPoint(aUID, aPoint));
  const BRepGraph_Transaction::CommitResult aCommit = anEdit.Commit();
  ASSERT_TRUE(aCommit.IsOk());
  EXPECT_TRUE(aCommit.Diff.IsEmpty());
  EXPECT_EQ(aCommit.Revision->SemanticHash(), aBase->SemanticHash());
  EXPECT_TRUE(anEdit.IsFinished());
  EXPECT_TRUE(anEdit.BaseRevision().IsNull());
  EXPECT_EQ(anEdit.Graph(), nullptr);
}

TEST(BRepGraph_RevisionTest, FailedCommitReleasesTransactionState)
{
  BRepGraph aGraph;
  ASSERT_TRUE(aGraph.Shapes().Add(BRepPrimAPI_MakeBox(1.0, 1.0, 1.0).Shape()).IsOk());
  const occ::handle<BRepGraph_Revision> aBase = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aBase.IsNull());
  ASSERT_FALSE(aBase->SupportsSparseEdits());

  BRepGraph_VertexIterator aVertices(aGraph);
  ASSERT_TRUE(aVertices.More());
  BRepGraph_Transaction anEdit        = aBase->BeginTransaction();
  BRepGraph*            aWorkingGraph = anEdit.Graph();
  ASSERT_NE(aWorkingGraph, nullptr);
  aWorkingGraph->Editor().Gen().RemoveNode(aVertices.CurrentId());
  const BRepGraph_Transaction::CommitResult aCommit = anEdit.Commit();
  EXPECT_EQ(aCommit.Status, BRepGraph_RevisionStatus::Code::ValidationFailed);
  EXPECT_TRUE(anEdit.IsFinished());
  EXPECT_TRUE(anEdit.BaseRevision().IsNull());
  EXPECT_EQ(anEdit.Graph(), nullptr);
}

TEST(BRepGraph_RevisionTest, CompactionPreservesSemanticHashButChangesPhysicalHash)
{
  BRepGraph                aGraph;
  const BRepGraph_VertexId aRemoved  = aGraph.Editor().Vertices().Add(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_VertexId aSurvivor = aGraph.Editor().Vertices().Add(gp_Pnt(1.0, 0.0, 0.0), 1.e-7);
  ASSERT_TRUE(aRemoved.IsValid() && aSurvivor.IsValid());

  aGraph.Editor().Gen().RemoveNode(BRepGraph_NodeId(aRemoved));
  aGraph.Editor().Gen().CleanupRemovedReferences();
  const occ::handle<BRepGraph_Revision> aBefore = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aBefore.IsNull());

  const BRepGraph_Compact::Result aCompactResult = BRepGraph_Compact::Perform(aGraph);
  EXPECT_EQ(aCompactResult.NbRemovedVertices, 1u);
  const occ::handle<BRepGraph_Revision> aAfter = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aAfter.IsNull());

  EXPECT_EQ(aBefore->SemanticHash(), aAfter->SemanticHash());
  EXPECT_NE(aBefore->StorageRootHash(), aAfter->StorageRootHash());
}

TEST(BRepGraph_RevisionTest, PackagePublishesAndRestoresEmptyRevision)
{
  const std::filesystem::path           aPath     = makePackageTestPath("empty");
  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::Empty();
  ASSERT_FALSE(aRevision.IsNull());

  const BRepGraphODE_RevisionPackage::Result aWrite =
    BRepGraphODE_RevisionPackage::Write(*aRevision, asOSDPath(aPath));
  ASSERT_TRUE(aWrite.IsOk()) << aWrite.Diagnostics.Size()
                             << (aWrite.Diagnostics.IsEmpty()
                                   ? ""
                                   : aWrite.Diagnostics.First().Message.ToCString());
  EXPECT_TRUE(std::filesystem::is_regular_file(aPath / "manifest.json"));
  EXPECT_TRUE(std::filesystem::is_regular_file(aPath / "revision.info"));

  const BRepGraphODE_RevisionPackage::Result aRead =
    BRepGraphODE_RevisionPackage::Read(asOSDPath(aPath));
  ASSERT_TRUE(aRead.IsOk()) << aRead.Diagnostics.Size();
  ASSERT_FALSE(aRead.Revision.IsNull());
  EXPECT_EQ(aRead.Revision->SemanticHash(), aRevision->SemanticHash());
  EXPECT_TRUE(aRead.Revision->Graph().IsEmpty());

  const BRepGraphODE_RevisionPackage::Result aValidate =
    BRepGraphODE_RevisionPackage::Validate(asOSDPath(aPath));
  EXPECT_TRUE(aValidate.IsOk()) << aValidate.Diagnostics.Size();
  EXPECT_TRUE(aValidate.Revision.IsNull());
  std::filesystem::remove_all(aPath);
}

TEST(BRepGraph_RevisionTest, PackagePreservesOriginalShapeBindings)
{
  BRepGraph                           aGraph;
  const TopoDS_Shape                  aBox    = BRepPrimAPI_MakeBox(2.0, 3.0, 4.0).Shape();
  const BRepGraph::ShapesView::Result anAdded = aGraph.Shapes().Add(aBox);
  ASSERT_TRUE(anAdded.IsOk());

  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aRevision.IsNull());
  ASSERT_TRUE(aRevision->Graph().Shapes().HasOriginal(anAdded.TopologyRoot));

  const std::filesystem::path                aPath = makePackageTestPath("original");
  const BRepGraphODE_RevisionPackage::Result aWrite =
    BRepGraphODE_RevisionPackage::Write(*aRevision, asOSDPath(aPath));
  ASSERT_TRUE(aWrite.IsOk()) << aWrite.Diagnostics.Size()
                             << (aWrite.Diagnostics.IsEmpty()
                                   ? ""
                                   : aWrite.Diagnostics.First().Message.ToCString());

  const BRepGraphODE_RevisionPackage::Result aRead =
    BRepGraphODE_RevisionPackage::Read(asOSDPath(aPath));
  ASSERT_TRUE(aRead.IsOk()) << aRead.Diagnostics.Size();
  ASSERT_FALSE(aRead.Revision.IsNull());
  const BRepGraph_NodeId aRestoredNode =
    aRead.Revision->Graph().UIDs().NodeIdFrom(aRevision->Graph().UIDs().Of(anAdded.TopologyRoot));
  ASSERT_TRUE(aRestoredNode.IsValid());
  EXPECT_TRUE(aRead.Revision->Graph().Shapes().HasOriginal(aRestoredNode));
  EXPECT_EQ(aRead.Revision->SemanticHash(), aRevision->SemanticHash());

  std::filesystem::remove_all(aPath);
}

TEST(BRepGraph_RevisionTest, PackageRoundTripsCompleteNativeRevision)
{
  BRepGraph_Transaction anEdit = BRepGraph_Revision::Empty()->BeginTransaction();
  ASSERT_TRUE(anEdit.IsValid());
  ASSERT_TRUE(anEdit.AddVertex(gp_Pnt(7.0, 8.0, 9.0), 1.e-7).IsValid());
  const BRepGraph_Transaction::CommitResult aCommit = anEdit.Commit();
  ASSERT_TRUE(aCommit.IsOk());

  const std::filesystem::path           aPath = makePackageTestPath("native-revision");
  BRepGraphODE_RevisionPackage::Options anOptions;
  const BRepGraphODE_RevisionPackage::Result aWrite =
    BRepGraphODE_RevisionPackage::Write(*aCommit.Revision, asOSDPath(aPath), anOptions);
  ASSERT_TRUE(aWrite.IsOk()) << (aWrite.Diagnostics.IsEmpty()
                                   ? ""
                                   : aWrite.Diagnostics.First().Message.ToCString());
  const BRepGraphODE_RevisionPackage::Result aRead =
    BRepGraphODE_RevisionPackage::Read(asOSDPath(aPath), anOptions);
  ASSERT_TRUE(aRead.IsOk()) << aRead.Diagnostics.Size();
  EXPECT_EQ(aRead.Revision->SemanticHash(), aCommit.Revision->SemanticHash());
  EXPECT_EQ(aRead.Revision->Graph().Topo().Vertices().NbActive(), 1u);
  BRepGraph_Transaction aRestoredEdit = aRead.Revision->BeginTransaction();
  ASSERT_TRUE(aRestoredEdit.IsValid());
  ASSERT_TRUE(aRestoredEdit.AddVertex(gp_Pnt(10.0, 11.0, 12.0), 1.e-7).IsValid());
  const BRepGraph_Transaction::CommitResult aRestoredCommit = aRestoredEdit.Commit();
  ASSERT_TRUE(aRestoredCommit.IsOk());
  EXPECT_EQ(aRestoredCommit.Revision->Graph().Topo().Vertices().NbActive(), 2u);
  std::filesystem::remove_all(aPath);
}

TEST(BRepGraph_RevisionTest, PackagePreparationIsNotReadableBeforePublish)
{
  const std::filesystem::path           aPath     = makePackageTestPath("prepared");
  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::Empty();
  ASSERT_FALSE(aRevision.IsNull());

  const BRepGraphODE_RevisionPackage::Result aPrepared =
    BRepGraphODE_RevisionPackage::Prepare(*aRevision, asOSDPath(aPath));
  ASSERT_TRUE(aPrepared.IsOk()) << aPrepared.Diagnostics.Size();
  EXPECT_FALSE(std::filesystem::exists(aPath));
  EXPECT_TRUE(std::filesystem::is_regular_file(asFilesystemPath(aPrepared.Prepared.StagingPath)
                                               / "manifest.json"));

  const BRepGraphODE_RevisionPackage::Result aBeforePublish =
    BRepGraphODE_RevisionPackage::Read(aPrepared.Prepared.PackagePath);
  EXPECT_FALSE(aBeforePublish.IsOk());
  EXPECT_EQ(aBeforePublish.StatusCode, BRepGraphODE_RevisionPackage::Status::NotFound);

  const BRepGraphODE_RevisionPackage::Result aPublished =
    BRepGraphODE_RevisionPackage::Publish(aPrepared.Prepared);
  ASSERT_TRUE(aPublished.IsOk()) << aPublished.Diagnostics.Size();
  EXPECT_TRUE(std::filesystem::is_regular_file(aPath / "manifest.json"));
  std::filesystem::remove_all(aPath);
  std::filesystem::remove_all(asFilesystemPath(aPrepared.Prepared.StagingPath));
}

TEST(BRepGraph_RevisionTest, PackagePublishCanRetryAfterTargetConflict)
{
  const std::filesystem::path           aPath     = makePackageTestPath("publish-retry");
  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::Empty();
  ASSERT_FALSE(aRevision.IsNull());
  const BRepGraphODE_RevisionPackage::Result aPrepared =
    BRepGraphODE_RevisionPackage::Prepare(*aRevision, asOSDPath(aPath));
  ASSERT_TRUE(aPrepared.IsOk());

  std::filesystem::create_directories(aPath);
  const BRepGraphODE_RevisionPackage::Result aConflict =
    BRepGraphODE_RevisionPackage::Publish(aPrepared.Prepared);
  EXPECT_EQ(aConflict.StatusCode, BRepGraphODE_RevisionPackage::Status::AlreadyExists);
  EXPECT_TRUE(std::filesystem::is_regular_file(asFilesystemPath(aPrepared.Prepared.StagingPath)
                                               / "manifest.json"));

  std::filesystem::remove_all(aPath);
  const BRepGraphODE_RevisionPackage::Result aPublished =
    BRepGraphODE_RevisionPackage::Publish(aPrepared.Prepared);
  ASSERT_TRUE(aPublished.IsOk()) << aPublished.Diagnostics.Size();
  const BRepGraphODE_RevisionPackage::Result aRetried =
    BRepGraphODE_RevisionPackage::Publish(aPrepared.Prepared);
  EXPECT_TRUE(aRetried.IsOk())
    << aRetried.Diagnostics.Size()
    << (aRetried.Diagnostics.IsEmpty() ? "" : aRetried.Diagnostics.First().Code.ToCString())
    << (aRetried.Diagnostics.IsEmpty() ? "" : aRetried.Diagnostics.First().Message.ToCString());
  std::filesystem::remove_all(aPath);
}

TEST(BRepGraph_RevisionTest, PackagePublishResumesAfterBackupRename)
{
  const std::filesystem::path           aPath     = makePackageTestPath("publish-backup-resume");
  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::Empty();
  ASSERT_FALSE(aRevision.IsNull());
  ASSERT_TRUE(BRepGraphODE_RevisionPackage::Write(*aRevision, asOSDPath(aPath)).IsOk());

  BRepGraphODE_RevisionPackage::Options anOptions;
  anOptions.ReplaceExisting = true;
  const BRepGraphODE_RevisionPackage::Result aPrepared =
    BRepGraphODE_RevisionPackage::Prepare(*aRevision, asOSDPath(aPath), anOptions);
  ASSERT_TRUE(aPrepared.IsOk());
  const std::filesystem::path aStage(asFilesystemPath(aPrepared.Prepared.StagingPath));
  std::filesystem::path       aBackup = asFilesystemPath(aPrepared.Prepared.StagingPath);
  aBackup += ".backup";
  std::filesystem::rename(aPath, aBackup);

  const BRepGraphODE_RevisionPackage::Result aPublished =
    BRepGraphODE_RevisionPackage::Publish(aPrepared.Prepared);
  ASSERT_TRUE(aPublished.IsOk())
    << aPublished.Diagnostics.Size()
    << (aPublished.Diagnostics.IsEmpty() ? "" : aPublished.Diagnostics.First().Code.ToCString())
    << (aPublished.Diagnostics.IsEmpty() ? "" : aPublished.Diagnostics.First().Message.ToCString());
  EXPECT_FALSE(std::filesystem::exists(aBackup));
  EXPECT_TRUE(BRepGraphODE_RevisionPackage::Validate(asOSDPath(aPath)).IsOk());
  std::filesystem::remove_all(aPath);
}

TEST(BRepGraph_RevisionTest, PackageRejectsModifiedRevisionDescription)
{
  const std::filesystem::path           aPath     = makePackageTestPath("manifest-canonical");
  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::Empty();
  ASSERT_FALSE(aRevision.IsNull());
  ASSERT_TRUE(BRepGraphODE_RevisionPackage::Write(*aRevision, asOSDPath(aPath)).IsOk());

  const std::filesystem::path aManifestPath = aPath / "revision.info";
  std::ofstream               aManifest(aManifestPath, std::ios::app);
  ASSERT_TRUE(aManifest);
  aManifest << "schema 1\n";
  aManifest.close();

  const BRepGraphODE_RevisionPackage::Result aRead =
    BRepGraphODE_RevisionPackage::Read(asOSDPath(aPath));
  EXPECT_EQ(aRead.StatusCode, BRepGraphODE_RevisionPackage::Status::DigestMismatch);
  EXPECT_TRUE(aRead.Revision.IsNull());
  std::filesystem::remove_all(aPath);
}

TEST(BRepGraph_RevisionTest, PackageRejectsCorruptedContentEntry)
{
  const std::filesystem::path           aPath     = makePackageTestPath("corrupt");
  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::Empty();
  ASSERT_FALSE(aRevision.IsNull());
  ASSERT_TRUE(BRepGraphODE_RevisionPackage::Write(*aRevision, asOSDPath(aPath)).IsOk());

  std::filesystem::path aCorePath;
  for (const std::filesystem::directory_entry& anEntry :
       std::filesystem::recursive_directory_iterator(aPath / "objects"))
  {
    if (anEntry.path().extension() == ".ode")
    {
      aCorePath = anEntry.path();
      break;
    }
  }
  ASSERT_FALSE(aCorePath.empty());
  std::ofstream aCorruptor(aCorePath, std::ios::binary | std::ios::app);
  ASSERT_TRUE(aCorruptor);
  aCorruptor << 'x';
  aCorruptor.close();

  const BRepGraphODE_RevisionPackage::Result aRead =
    BRepGraphODE_RevisionPackage::Read(asOSDPath(aPath));
  EXPECT_EQ(aRead.StatusCode, BRepGraphODE_RevisionPackage::Status::DigestMismatch);
  EXPECT_TRUE(aRead.Revision.IsNull());
  std::filesystem::remove_all(aPath);
}

TEST(BRepGraph_RevisionTest, PackageRejectsSymlinkedContentEntry)
{
  const std::filesystem::path           aPath     = makePackageTestPath("symlink");
  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::Empty();
  ASSERT_FALSE(aRevision.IsNull());
  ASSERT_TRUE(BRepGraphODE_RevisionPackage::Write(*aRevision, asOSDPath(aPath)).IsOk());

  std::filesystem::path aCorePath;
  for (const std::filesystem::directory_entry& anEntry :
       std::filesystem::recursive_directory_iterator(aPath / "objects"))
  {
    if (anEntry.path().extension() == ".ode")
    {
      aCorePath = anEntry.path();
      break;
    }
  }
  ASSERT_FALSE(aCorePath.empty());

  const std::filesystem::path anOutsidePath = aPath.string() + "-outside.ode";
  std::error_code             anError;
  std::filesystem::remove(anOutsidePath, anError);
  anError.clear();
  std::filesystem::rename(aCorePath, anOutsidePath, anError);
  ASSERT_FALSE(anError);
  std::filesystem::create_symlink(anOutsidePath, aCorePath, anError);
  if (anError)
  {
    std::filesystem::remove_all(aPath);
    std::filesystem::remove(anOutsidePath);
    GTEST_SKIP() << "Symbolic links are unavailable: " << anError.message();
  }
  ASSERT_TRUE(std::filesystem::is_symlink(std::filesystem::symlink_status(aCorePath)));

  const BRepGraphODE_RevisionPackage::Result aRead =
    BRepGraphODE_RevisionPackage::Read(asOSDPath(aPath));
  ASSERT_EQ(aRead.StatusCode, BRepGraphODE_RevisionPackage::Status::Incomplete)
    << (aRead.Diagnostics.IsEmpty() ? "" : aRead.Diagnostics.First().Code.ToCString());
  EXPECT_TRUE(aRead.Revision.IsNull());
  std::filesystem::remove_all(aPath);
  std::filesystem::remove(anOutsidePath);
}

TEST(BRepGraph_RevisionTest, PackageDoesNotPersistRuntimeFingerprints)
{
  const std::filesystem::path           aPath     = makePackageTestPath("runtime-fingerprint");
  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::Empty();
  ASSERT_FALSE(aRevision.IsNull());
  ASSERT_TRUE(BRepGraphODE_RevisionPackage::Write(*aRevision, asOSDPath(aPath)).IsOk());

  const std::filesystem::path aManifestPath = aPath / "revision.info";
  std::ifstream               aInput(aManifestPath);
  ASSERT_TRUE(aInput);
  std::string aManifest;
  std::string aLine;
  while (std::getline(aInput, aLine))
  {
    aManifest += aLine;
    aManifest += '\n';
  }
  EXPECT_EQ(aManifest.find("\nsemantic "), std::string::npos);
  EXPECT_EQ(aManifest.find("\nstorage "), std::string::npos);
  EXPECT_EQ(aManifest.find(aRevision->SemanticHash().ToString().ToCString()), std::string::npos);
  EXPECT_EQ(aManifest.find(aRevision->StorageRootHash().ToString().ToCString()), std::string::npos);
  std::filesystem::remove_all(aPath);
}

TEST(BRepGraph_RevisionTest, PackagedTombstoneRevisionRetainsSparseEditability)
{
  BRepGraph                aGraph;
  const BRepGraph_VertexId aVertex = aGraph.Editor().Vertices().Add(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  ASSERT_TRUE(aVertex.IsValid());
  aGraph.Editor().Gen().RemoveNode(BRepGraph_NodeId(aVertex));
  aGraph.Editor().Gen().CleanupRemovedReferences();

  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aRevision.IsNull());
  ASSERT_TRUE(aRevision->SupportsSparseEdits());

  const std::filesystem::path aPath = makePackageTestPath("tombstone");
  ASSERT_TRUE(BRepGraphODE_RevisionPackage::Write(*aRevision, asOSDPath(aPath)).IsOk());
  const BRepGraphODE_RevisionPackage::Result aRead =
    BRepGraphODE_RevisionPackage::Read(asOSDPath(aPath));
  ASSERT_TRUE(aRead.IsOk()) << aRead.Diagnostics.Size();
  ASSERT_FALSE(aRead.Revision.IsNull());
  EXPECT_TRUE(aRead.Revision->SupportsSparseEdits());
  EXPECT_TRUE(aRead.Revision->BeginTransaction().IsValid());
  std::filesystem::remove_all(aPath);
}

TEST(BRepGraph_RevisionTest, DiffReportsEdgeCurveDifference)
{
  BRepGraph                aBaseGraph;
  const BRepGraph_VertexId aBaseStart =
    aBaseGraph.Editor().Vertices().Add(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_VertexId aBaseEnd =
    aBaseGraph.Editor().Vertices().Add(gp_Pnt(1.0, 0.0, 0.0), 1.e-7);
  ASSERT_TRUE(aBaseStart.IsValid() && aBaseEnd.IsValid());
  const occ::handle<Geom_Curve> aBaseCurve =
    new Geom_Line(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0));
  ASSERT_TRUE(
    aBaseGraph.Editor().Edges().Add(aBaseStart, aBaseEnd, aBaseCurve, 0.0, 1.0, 1.e-7).IsValid());

  BRepGraph                aResultGraph;
  const BRepGraph_VertexId aResultStart =
    aResultGraph.Editor().Vertices().Add(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_VertexId aResultEnd =
    aResultGraph.Editor().Vertices().Add(gp_Pnt(1.0, 0.0, 0.0), 1.e-7);
  ASSERT_TRUE(aResultStart.IsValid() && aResultEnd.IsValid());
  const occ::handle<Geom_Curve> aResultCurve =
    new Geom_Line(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 1.0, 0.0));
  ASSERT_TRUE(aResultGraph.Editor()
                .Edges()
                .Add(aResultStart, aResultEnd, aResultCurve, 0.0, 1.0, 1.e-7)
                .IsValid());

  const occ::handle<BRepGraph_Revision> aBase   = BRepGraph_Revision::FromGraph(aBaseGraph);
  const occ::handle<BRepGraph_Revision> aResult = BRepGraph_Revision::FromGraph(aResultGraph);
  ASSERT_FALSE(aBase.IsNull());
  ASSERT_FALSE(aResult.IsNull());
  const BRepGraph_RevisionDiff aDiff = aBase->Diff(*aResult);
  ASSERT_EQ(aDiff.ModifiedUIDs.Size(), 1u);
  EXPECT_EQ(aDiff.ModifiedUIDs.First().Kind, BRepGraph_NodeId::Kind::Edge);
}

TEST(BRepGraph_RevisionTest, DiffReportsEdgeParameterDifference)
{
  BRepGraph                aBaseGraph;
  const BRepGraph_VertexId aBaseStart =
    aBaseGraph.Editor().Vertices().Add(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_VertexId aBaseEnd =
    aBaseGraph.Editor().Vertices().Add(gp_Pnt(1.0, 0.0, 0.0), 1.e-7);
  const occ::handle<Geom_Curve> aBaseCurve =
    new Geom_Line(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0));
  ASSERT_TRUE(
    aBaseGraph.Editor().Edges().Add(aBaseStart, aBaseEnd, aBaseCurve, 0.0, 1.0, 1.e-7).IsValid());

  BRepGraph                aResultGraph;
  const BRepGraph_VertexId aResultStart =
    aResultGraph.Editor().Vertices().Add(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_VertexId aResultEnd =
    aResultGraph.Editor().Vertices().Add(gp_Pnt(1.0, 0.0, 0.0), 1.e-7);
  const occ::handle<Geom_Curve> aResultCurve =
    new Geom_Line(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0));
  ASSERT_TRUE(aResultGraph.Editor()
                .Edges()
                .Add(aResultStart, aResultEnd, aResultCurve, 0.25, 1.0, 1.e-7)
                .IsValid());

  const occ::handle<BRepGraph_Revision> aBase   = BRepGraph_Revision::FromGraph(aBaseGraph);
  const occ::handle<BRepGraph_Revision> aResult = BRepGraph_Revision::FromGraph(aResultGraph);
  ASSERT_FALSE(aBase.IsNull());
  ASSERT_FALSE(aResult.IsNull());
  const BRepGraph_RevisionDiff aDiff = aBase->Diff(*aResult);
  ASSERT_EQ(aDiff.ModifiedUIDs.Size(), 1u);
  EXPECT_EQ(aDiff.ModifiedUIDs.First().Kind, BRepGraph_NodeId::Kind::Edge);
}

TEST(BRepGraph_RevisionTest, TopologyOnlyWireFaceEditPreservesOrderedMembership)
{
  BRepGraph_Transaction aVertexEdit = BRepGraph_Revision::Empty()->BeginTransaction();
  ASSERT_TRUE(aVertexEdit.IsValid());
  const BRepGraph_VertexId aVertex0 = aVertexEdit.AddVertex(gp_Pnt(0.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_VertexId aVertex1 = aVertexEdit.AddVertex(gp_Pnt(1.0, 0.0, 0.0), 1.e-7);
  const BRepGraph_VertexId aVertex2 = aVertexEdit.AddVertex(gp_Pnt(0.0, 1.0, 0.0), 1.e-7);
  ASSERT_TRUE(aVertex0.IsValid() && aVertex1.IsValid() && aVertex2.IsValid());
  const BRepGraph_Transaction::CommitResult aVertexCommit = aVertexEdit.Commit();
  ASSERT_TRUE(aVertexCommit.IsOk());

  const BRepGraph_UID   aVertexUID0 = aVertexCommit.Revision->Graph().UIDs().Of(aVertex0);
  const BRepGraph_UID   aVertexUID1 = aVertexCommit.Revision->Graph().UIDs().Of(aVertex1);
  const BRepGraph_UID   aVertexUID2 = aVertexCommit.Revision->Graph().UIDs().Of(aVertex2);
  BRepGraph_Transaction anEdgeEdit  = aVertexCommit.Revision->BeginTransaction();
  ASSERT_TRUE(anEdgeEdit.IsValid());
  const occ::handle<Geom_Curve> aCurve =
    new Geom_Line(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0));
  const BRepGraph_EdgeId anEdge0 =
    anEdgeEdit.AddEdge(aVertexUID0, aVertexUID1, aCurve, 0.0, 1.0, 1.e-7);
  const BRepGraph_EdgeId anEdge1 =
    anEdgeEdit.AddEdge(aVertexUID1, aVertexUID2, aCurve, 0.0, 1.0, 1.e-7);
  const BRepGraph_EdgeId anEdge2 =
    anEdgeEdit.AddEdge(aVertexUID2, aVertexUID0, aCurve, 0.0, 1.0, 1.e-7);
  ASSERT_TRUE(anEdge0.IsValid() && anEdge1.IsValid() && anEdge2.IsValid());
  const BRepGraph_Transaction::CommitResult anEdgeCommit = anEdgeEdit.Commit();
  ASSERT_TRUE(anEdgeCommit.IsOk());

  BRepGraph_Transaction aCoEdgeEdit = anEdgeCommit.Revision->BeginTransaction();
  ASSERT_TRUE(aCoEdgeEdit.IsValid());
  const BRepGraph_UID      anEdgeUID0 = anEdgeCommit.Revision->Graph().UIDs().Of(anEdge0);
  const BRepGraph_UID      anEdgeUID1 = anEdgeCommit.Revision->Graph().UIDs().Of(anEdge1);
  const BRepGraph_UID      anEdgeUID2 = anEdgeCommit.Revision->Graph().UIDs().Of(anEdge2);
  const BRepGraph_CoEdgeId aCoEdge0   = aCoEdgeEdit.AddCoEdge(anEdgeUID0, TopAbs_FORWARD);
  const BRepGraph_CoEdgeId aCoEdge1   = aCoEdgeEdit.AddCoEdge(anEdgeUID1, TopAbs_FORWARD);
  const BRepGraph_CoEdgeId aCoEdge2   = aCoEdgeEdit.AddCoEdge(anEdgeUID2, TopAbs_FORWARD);
  ASSERT_TRUE(aCoEdge0.IsValid() && aCoEdge1.IsValid() && aCoEdge2.IsValid());
  const BRepGraph_Transaction::CommitResult aCoEdgeCommit = aCoEdgeEdit.Commit();
  ASSERT_TRUE(aCoEdgeCommit.IsOk());

  NCollection_LinearVector<BRepGraph_UID> aCoEdgeUIDs;
  aCoEdgeUIDs.Append(aCoEdgeCommit.Revision->Graph().UIDs().Of(aCoEdge0));
  aCoEdgeUIDs.Append(aCoEdgeCommit.Revision->Graph().UIDs().Of(aCoEdge1));
  aCoEdgeUIDs.Append(aCoEdgeCommit.Revision->Graph().UIDs().Of(aCoEdge2));
  BRepGraph_Transaction aTopologyEdit = aCoEdgeCommit.Revision->BeginTransaction();
  ASSERT_TRUE(aTopologyEdit.IsValid());
  const BRepGraph_WireId aWire = aTopologyEdit.AddWire(aCoEdgeUIDs);
  ASSERT_TRUE(aWire.IsValid());
  BRepGraph_UID aWireUID;
  ASSERT_TRUE(aTopologyEdit.WireUID(aWire, aWireUID));
  NCollection_LinearVector<BRepGraph_UID> anEmptyInnerWires;
  const BRepGraph_FaceId aFace = aTopologyEdit.AddFace(aWireUID, anEmptyInnerWires, 2.e-7);
  ASSERT_TRUE(aFace.IsValid());

  const BRepGraph_Transaction::CommitResult aTopologyCommit = aTopologyEdit.Commit();
  ASSERT_TRUE(aTopologyCommit.IsOk())
    << (aTopologyCommit.Diagnostics.IsEmpty()
          ? ""
          : aTopologyCommit.Diagnostics.First().Message.ToCString());
  expectFreshCoreHashes(aTopologyCommit.Revision);
  ASSERT_TRUE(aTopologyCommit.Revision->SupportsSparseEdits());
  EXPECT_EQ(aTopologyCommit.Revision->NbVisibleWires(), 1u);
  EXPECT_EQ(aTopologyCommit.Revision->NbVisibleFaces(), 1u);
  EXPECT_EQ(aTopologyCommit.Revision->NbVisibleWireRefs(), 1u);
  EXPECT_TRUE(aTopologyCommit.Revision->Graph().ValidateRelations());

  const BRepGraph_UID aFaceUID                = aTopologyCommit.Revision->Graph().UIDs().Of(aFace);
  const occ::handle<BRepGraph_Revision> aRead = aTopologyCommit.Revision;
  BRepGraph_Revision::WireChange        aWireChange;
  ASSERT_TRUE(aRead->ReadWire(aWireUID, aWireChange));
  ASSERT_EQ(aWireChange.CoEdgeUIDs.Size(), aCoEdgeUIDs.Size());
  for (size_t anIndex = 0; anIndex < aCoEdgeUIDs.Size(); ++anIndex)
  {
    EXPECT_EQ(aWireChange.CoEdgeUIDs.Value(anIndex), aCoEdgeUIDs.Value(anIndex));
  }
  BRepGraph_Revision::FaceChange aFaceChange;
  ASSERT_TRUE(aRead->ReadFace(aFaceUID, aFaceChange));
  ASSERT_EQ(aFaceChange.WireRefUIDs.Size(), 1u);
  BRepGraph_Revision::WireRefChange aWireRefChange;
  ASSERT_TRUE(aRead->ReadWireRef(aFaceChange.WireRefUIDs.First(), aWireRefChange));
  EXPECT_EQ(aWireRefChange.Role, BRepGraph_Revision::BoundaryRole::Outer);
  EXPECT_EQ(aTopologyCommit.Revision->SemanticHash(),
            BRepGraph_RevisionHash::Hasher::Semantic(aTopologyCommit.Revision->Graph()));

  const BRepGraph_RevisionDiff aDiff = aCoEdgeCommit.Revision->Diff(*aTopologyCommit.Revision);
  EXPECT_NE(std::find(aDiff.CreatedUIDs.begin(), aDiff.CreatedUIDs.end(), aWireUID),
            aDiff.CreatedUIDs.end());
  EXPECT_NE(std::find(aDiff.CreatedUIDs.begin(), aDiff.CreatedUIDs.end(), aFaceUID),
            aDiff.CreatedUIDs.end());
  EXPECT_EQ(aDiff.CreatedRefUIDs.Size(), 1u);

  BRepGraph_Transaction aBoundCoEdgeRemoval = aTopologyCommit.Revision->BeginTransaction();
  ASSERT_TRUE(aBoundCoEdgeRemoval.IsValid());
  EXPECT_FALSE(aBoundCoEdgeRemoval.RemoveCoEdge(aCoEdgeUIDs.First()));
  aBoundCoEdgeRemoval.Abort();

  const std::filesystem::path           anPackagePath = makePackageTestPath("wire-face");
  BRepGraphODE_RevisionPackage::Options anPackageOptions;
  const BRepGraphODE_RevisionPackage::Result anPackageWrite =
    BRepGraphODE_RevisionPackage::Write(*aTopologyCommit.Revision,
                                        asOSDPath(anPackagePath),
                                        anPackageOptions);
  ASSERT_TRUE(anPackageWrite.IsOk())
    << (anPackageWrite.Diagnostics.IsEmpty()
          ? ""
          : anPackageWrite.Diagnostics.First().Message.ToCString());
  const BRepGraphODE_RevisionPackage::Result anPackageRead =
    BRepGraphODE_RevisionPackage::Read(asOSDPath(anPackagePath), anPackageOptions);
  ASSERT_TRUE(anPackageRead.IsOk()) << anPackageRead.Diagnostics.Size();
  EXPECT_EQ(anPackageRead.Revision->SemanticHash(), aTopologyCommit.Revision->SemanticHash());
  BRepGraph_Revision::FaceChange anPackagedFaceChange;
  EXPECT_TRUE(anPackageRead.Revision->ReadFace(aFaceUID, anPackagedFaceChange));

  BRepGraph_Transaction aRestoredEdit = anPackageRead.Revision->BeginTransaction();
  ASSERT_TRUE(aRestoredEdit.IsValid());
  ASSERT_TRUE(aRestoredEdit.AddVertex(gp_Pnt(2.0, 2.0, 0.0), 1.e-7).IsValid());
  const BRepGraph_Transaction::CommitResult aRestoredCommit = aRestoredEdit.Commit();
  ASSERT_TRUE(aRestoredCommit.IsOk());
  BRepGraph_Revision::FaceChange aRestoredFaceChange;
  EXPECT_TRUE(aRestoredCommit.Revision->ReadFace(aFaceUID, aRestoredFaceChange));
  std::filesystem::remove_all(anPackagePath);
}

TEST(BRepGraph_RevisionTest, FaceWireRefRemovalPreservesOuterBoundaryInvariant)
{
  BRepGraph_Transaction anEdit = BRepGraph_Revision::Empty()->BeginTransaction();
  ASSERT_TRUE(anEdit.IsValid());
  NCollection_LinearVector<BRepGraph_UID> anEmptyCoEdges;
  const BRepGraph_WireId                  anOuterWire = anEdit.AddWire(anEmptyCoEdges);
  const BRepGraph_WireId                  anInnerWire = anEdit.AddWire(anEmptyCoEdges);
  ASSERT_TRUE(anOuterWire.IsValid() && anInnerWire.IsValid());

  BRepGraph_UID anOuterWireUID;
  BRepGraph_UID anInnerWireUID;
  ASSERT_TRUE(anEdit.WireUID(anOuterWire, anOuterWireUID));
  ASSERT_TRUE(anEdit.WireUID(anInnerWire, anInnerWireUID));
  NCollection_LinearVector<BRepGraph_UID> anInnerWires;
  anInnerWires.Append(anInnerWireUID);
  const BRepGraph_FaceId aFace = anEdit.AddFace(anOuterWireUID, anInnerWires, 1.e-7);
  ASSERT_TRUE(aFace.IsValid());
  const BRepGraph_Transaction::CommitResult aCommit = anEdit.Commit();
  ASSERT_TRUE(aCommit.IsOk());

  const BRepGraph_UID            aFaceUID = aCommit.Revision->Graph().UIDs().Of(aFace);
  BRepGraph_Revision::FaceChange aFaceChange;
  ASSERT_TRUE(aCommit.Revision->ReadFace(aFaceUID, aFaceChange));
  ASSERT_EQ(aFaceChange.WireRefUIDs.Size(), 2u);
  BRepGraph_Revision::WireRefChange anOuterWireRefChange;
  BRepGraph_Revision::WireRefChange anInnerWireRefChange;
  ASSERT_TRUE(aCommit.Revision->ReadWireRef(aFaceChange.WireRefUIDs.First(), anOuterWireRefChange));
  ASSERT_TRUE(
    aCommit.Revision->ReadWireRef(aFaceChange.WireRefUIDs.Value(1), anInnerWireRefChange));
  EXPECT_EQ(anOuterWireRefChange.Role, BRepGraph_Revision::BoundaryRole::Outer);
  EXPECT_EQ(anInnerWireRefChange.Role, BRepGraph_Revision::BoundaryRole::Inner);

  BRepGraph_Transaction anInnerRemoval = aCommit.Revision->BeginTransaction();
  ASSERT_TRUE(anInnerRemoval.IsValid());
  ASSERT_TRUE(anInnerRemoval.RemoveWireRef(aFaceChange.WireRefUIDs.Value(1)));
  EXPECT_TRUE(anInnerRemoval.Commit().IsOk());

  BRepGraph_Transaction anOuterRemoval = aCommit.Revision->BeginTransaction();
  ASSERT_TRUE(anOuterRemoval.IsValid());
  EXPECT_FALSE(anOuterRemoval.RemoveWireRef(aFaceChange.WireRefUIDs.First()));
}

TEST(BRepGraph_RevisionTest, WireRemovalRejectsPendingFaceReference)
{
  BRepGraph_Transaction anEdit = BRepGraph_Revision::Empty()->BeginTransaction();
  ASSERT_TRUE(anEdit.IsValid());

  NCollection_LinearVector<BRepGraph_UID> anEmptyCoEdges;
  const BRepGraph_WireId                  aWire = anEdit.AddWire(anEmptyCoEdges);
  ASSERT_TRUE(aWire.IsValid());

  BRepGraph_UID aWireUID;
  ASSERT_TRUE(anEdit.WireUID(aWire, aWireUID));
  NCollection_LinearVector<BRepGraph_UID> anEmptyInnerWires;
  ASSERT_TRUE(anEdit.AddFace(aWireUID, anEmptyInnerWires, 1.e-7).IsValid());

  EXPECT_FALSE(anEdit.RemoveWire(aWireUID));
  const BRepGraph_Transaction::CommitResult aCommit = anEdit.Commit();
  ASSERT_TRUE(aCommit.IsOk());
  EXPECT_TRUE(aCommit.Revision->Graph().ValidateRelations());
}

TEST(BRepGraph_ReplaceTest, RestoresPersistentRevisionInStableWrapper)
{
  BRepGraph                      aGraph;
  BRepGraph::ShapesView::Options anOptions;
  anOptions.Parallel = false;
  ASSERT_TRUE(aGraph.Shapes().Add(BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape(), anOptions).IsOk());
  const occ::handle<BRepGraph_Revision> aRevision = BRepGraph_Revision::FromGraph(aGraph);
  ASSERT_FALSE(aRevision.IsNull());

  BRepGraph_VertexId aVertex = BRepGraph_VertexId::Start();
  ASSERT_TRUE(aVertex.IsValid(aGraph.Topo().Vertices().Nb()));
  {
    BRepGraph_MutGuard<BRepGraphInc::VertexDef> aChange = aGraph.Editor().Vertices().Mut(aVertex);
    const gp_Pnt                                aPoint  = aChange->Point;
    aGraph.Editor().Vertices().SetPoint(aChange, gp_Pnt(aPoint.X() + 5.0, aPoint.Y(), aPoint.Z()));
  }
  EXPECT_NE(BRepGraph_RevisionHash::Hasher::Semantic(aGraph), aRevision->SemanticHash());

  BRepGraph aReplacement;
  ASSERT_TRUE(aRevision->CopyTo(aReplacement));
  BRepGraph* const                aStableAddress = &aGraph;
  const BRepGraph_Replace::Result aResult =
    BRepGraph_Replace::Perform(aGraph, std::move(aReplacement));
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(&aGraph, aStableAddress);
  EXPECT_EQ(BRepGraph_RevisionHash::Hasher::Semantic(aGraph), aRevision->SemanticHash());
}

TEST(BRepGraph_ReplaceTest, RejectsDifferentGraphIdentityWithoutMutation)
{
  BRepGraph                      aGraph;
  BRepGraph                      aForeignGraph;
  BRepGraph::ShapesView::Options anOptions;
  anOptions.Parallel = false;
  ASSERT_TRUE(aGraph.Shapes().Add(BRepPrimAPI_MakeBox(1.0, 2.0, 3.0).Shape(), anOptions).IsOk());
  ASSERT_TRUE(
    aForeignGraph.Shapes().Add(BRepPrimAPI_MakeBox(4.0, 5.0, 6.0).Shape(), anOptions).IsOk());
  const BRepGraph_RevisionHash aBefore = BRepGraph_RevisionHash::Hasher::Semantic(aGraph);

  const BRepGraph_Replace::Result aResult =
    BRepGraph_Replace::Perform(aGraph, std::move(aForeignGraph));
  EXPECT_EQ(aResult.StatusCode, BRepGraph_Replace::Status::DifferentGraphIdentity);
  EXPECT_EQ(BRepGraph_RevisionHash::Hasher::Semantic(aGraph), aBefore);
}
